#include "LevelRenderer.hpp"

#include "Templates.hpp"
#include "Level/Level.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace nbl
{
    LevelRenderer::LevelRenderer(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager, Level* pLevel)
    : mRHI(rhi)
    , mTextureManager(pTextureManager)
    , mLevel(pLevel)
    {
        using enum vk::ImageUsageFlagBits;
        mAlbedoBuffer = mRHI->createImage({
            .extent        = mRHI->getSwapchain()->getProperties().extent,
            .format        = vk::Format::eR16G16B16A16Sfloat,
            .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
            .debugName     = "Indirect_GBuffer_AlbedoBuffer"
        });
        mDepthBuffer = mRHI->createImage({
            .extent        = mRHI->getSwapchain()->getProperties().extent,
            .format        = vk::Format::eD32Sfloat,
            .usageFlags    = eDepthStencilAttachment | eSampled | eTransferSrc | eTransferDst,
            .debugName     = "Indirect_GBuffer_DepthBuffer"
        });

        mRenderPass = mRHI->createRenderPass({
            .renderArea = {{0, 0}, mRHI->getSwapchain()->getProperties().extent },
            .colorAttachments = { makeAttachment(mAlbedoBuffer.get()) },
            .depthAttachment = makeAttachment(mDepthBuffer.get()),
            .label = "GBufferPass",
        });

        const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .addDescriptorSetLayout(mTextureManager->getDescriptor()->getLayout())
            .setPushConstantRange({ vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstants) })
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .configure([](RHI::GraphicsPipelineStateInfo& stateInfo) {
                    stateInfo.addAttributeDescriptions<Vertex>(0, 0);
                    stateInfo.addBindingDescriptions<Vertex>(0);
                })
                .addDefaultAttachmentStates(1))
            .addShader({ Configuration::getShaderFilePath("TestNew.vert.spv"), vk::ShaderStageFlagBits::eVertex })
            .addShader({ Configuration::getShaderFilePath("TestNew.frag.spv"), vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mRHI->getSwapchain()->getProperties().format)
            .setDepthAttachmentFormat(mDepthBuffer->getProperties().format)
            .setDebugName("Indirect_GBuffer_Pipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);

        mScissor = getRenderAreaForAttachment(mAlbedoBuffer.get());
        mViewport = vk::Viewport {
            0.0f, 0.0f,
            static_cast<float>(mScissor.extent.width), static_cast<float>(mScissor.extent.height),
            0.0f, 1.0f
        };

        mBoundingBoxDebugPass = BoundingBoxDebugPass::create({
            .pLevel             = mLevel,
            .renderTarget       = mAlbedoBuffer,
            .gBufferDepthBuffer = mDepthBuffer,
            .rhi                = mRHI,
        });
    }

    void LevelRenderer::render(const RHI::FrameData& frameData, const RHI::CommandList* commandList) const
    {
        RHI::Barrier()
            .addBarrier(mAlbedoBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mDepthBuffer->getBarrier(RHI::ImageUsage::DepthAttachment))
            .addBarrier(mLevel->mInstanceSystem->getBuffer()->getBarrier(RHI::BufferUsage::Compute_Read, RHI::BufferUsage::StorageRead))
            .addBarrier(mLevel->mInstanceIndirectionMapBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::StorageRead))
            .addBarrier(mLevel->mDrawCommandsBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::DrawIndirect))
            .insert(commandList);

        mRenderPass->execute(commandList, [&](const RHI::CommandList* cmd) -> void {
            cmd->setViewportScissor(mViewport, mScissor);

            const PushConstants pc = {
                .instanceBufferAddress = mLevel->mInstanceSystem->getBuffer()->getAddress(),
                .instanceMapAddress    = mLevel->mInstanceIndirectionMapBuffer[frameData.currentFrame]->getAddress(),
                .cameraUniformAddress  = mLevel->mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
                .materialAddress       = mLevel->mMaterialSystem->getBuffer()->getAddress(),
            };

            mPipeline->bind(cmd);
            mPipeline->bindDescriptorSet(cmd, mTextureManager->getDescriptor()->getSet(0));
            mPipeline->pushConstants(cmd, &pc);

            static constexpr vk::DeviceSize offsets[1] = { 0 };
            const auto& buffers = mLevel->mGeometrySystem->getBuffers();

            const std::array vertexBuffers { buffers.getVertexBuffer()->getHandle() };
            cmd->getHandle().bindVertexBuffers(0, 1, vertexBuffers.data(), offsets);
            cmd->getHandle().bindIndexBuffer(buffers.getIndexBuffer()->getHandle(), 0, vk::IndexType::eUint32);

            mLevel->drawIndexedIndirect(cmd, frameData);
        });

        mBoundingBoxDebugPass->execute(commandList, frameData);

        blitToSwapchain(mAlbedoBuffer.get(), commandList, frameData);
    }

    void LevelRenderer::blitToSwapchain(RHI::Image* pImage, const RHI::CommandList* commandList, const RHI::FrameData& frameData) const
    {
        commandList->beginLabel("Present_Blit");
        // Barriers
        const auto barrier = RHI::Barrier()
            .addBarrier(pImage->getBarrier(RHI::ImageUsage::TransferSrc))
            .addBarrier(mRHI->getSwapchain()->getBarrier(frameData.acquiredIndex, RHI::ImageUsage::TransferDst));
        barrier.insert(commandList);

        // Blit
        #pragma region
        const auto srcExtent = pImage->getProperties().extent;
        const auto dstExtent = mRHI->getSwapchain()->getProperties().extent;
        const auto region  = vk::ImageBlit2()
            .setSrcOffsets({
                vk::Offset3D { 0, 0, 0 },
                vk::Offset3D { static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), 1 }
            })
            .setSrcSubresource(pImage->getProperties().getSubresourceLayers())
            .setDstOffsets({
                vk::Offset3D { 0, 0, 0 },
                vk::Offset3D { static_cast<int32_t>(dstExtent.width), static_cast<int32_t>(dstExtent.height), 1 }
            })
            .setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });

        const auto blit = vk::BlitImageInfo2()
            .setSrcImage(pImage->getImage())
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setDstImage(mRHI->getSwapchain()->getImage(frameData.acquiredIndex))
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setFilter(vk::Filter::eLinear)
            .setRegions(region);
        #pragma endregion
        commandList->getHandle().blitImage2(blit);

        commandList->endLabel();
    }
}
