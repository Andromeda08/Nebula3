#include "GBufferPass.hpp"

#include "Templates.hpp"

namespace nbl
{
    GBufferPass::GBufferPass(const GBufferPass_Params& params)
    : mRHI(params.rhi)
    , mInput(params)
    {
        init();
    }

    void GBufferPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        pCommandList->beginLabel("GBufferPass");

        const auto* level = mInput.pLevel;
        RHI::Barrier()
            .addBarrier(mWorldPosition->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mWorldNormal->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mMotionVectors->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mViewZ->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mAlbedoBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mEmissiveBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mParamsBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mDepthBuffer->getBarrier(RHI::ImageUsage::DepthAttachment))
            .addBarrier(level->mInstanceSystem->getBuffer()->getBarrier(RHI::BufferUsage::Compute_Read, RHI::BufferUsage::StorageRead))
            .addBarrier(level->mInstanceIndirectionMapBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::StorageRead))
            .addBarrier(level->mDrawCommandsBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::DrawIndirect))
            .insert(pCommandList);

        mRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
            cmd->setViewportScissor(mViewport, mScissor);

            const auto [w, h] = mWorldPosition->getProperties().extent;
            const PushConstants pc = {
                .instanceBuffer            = level->mInstanceSystem->getBuffer()->getAddress(),
                .instanceIndirectionBuffer = level->mInstanceIndirectionMapBuffer[frameData.currentFrame]->getAddress(),
                .cameraBuffer              = level->mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
                .previousCameraBuffer      = level->mCameraSystem->getPreviousBuffer(frameData.currentFrame)->getAddress(),
                .materialBuffer            = level->mMaterialSystem->getBuffer()->getAddress(),
                .renderRes                 = glm::uvec2(w, h),
            };

            mPipeline->bind(cmd);
            mPipeline->bindDescriptorSet(cmd, mInput.pTextureManager->getDescriptor()->getSet(0));
            mPipeline->pushConstants(cmd, &pc);

            static constexpr vk::DeviceSize offsets[1] = { 0 };
            const auto& buffers = level->mGeometrySystem->getBuffers();

            const std::array vertexBuffers { buffers.getVertexBuffer()->getHandle() };
            cmd->getHandle().bindVertexBuffers(0, 1, vertexBuffers.data(), offsets);
            cmd->getHandle().bindIndexBuffer(buffers.getIndexBuffer()->getHandle(), 0, vk::IndexType::eUint32);

            level->drawIndexedIndirect(cmd, frameData);
        });

        pCommandList->endLabel();
    }

    const SPtr<RHI::Image>& GBufferPass::getAlbedoBuffer() const noexcept
    {
        return mAlbedoBuffer;
    }

    const SPtr<RHI::Image>& GBufferPass::getDepthBuffer() const noexcept
    {
        return mDepthBuffer;
    }

    void GBufferPass::init() noexcept
    {
        constexpr auto samplerInfo = vk::SamplerCreateInfo()
            .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
            .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
            .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
            .setMinFilter(vk::Filter::eNearest)
            .setMagFilter(vk::Filter::eNearest);

        mWorldPosition  = makeRenderTarget(mRHI.get(), "GBuffer_WorldPosition", vk::Format::eR32G32B32A32Sfloat, std::nullopt, samplerInfo);
        mWorldNormal    = makeRenderTarget(mRHI.get(), "GBuffer_WorldNormal",   vk::Format::eR32G32B32A32Sfloat, std::nullopt, samplerInfo);
        mMotionVectors  = makeRenderTarget(mRHI.get(), "GBuffer_MV_DeltaViewZ", vk::Format::eR32G32B32A32Sfloat, std::nullopt, samplerInfo);
        mViewZ          = makeRenderTarget(mRHI.get(), "GBuffer_ViewZ",         vk::Format::eR32Sfloat,          std::nullopt, samplerInfo);
        mAlbedoBuffer   = makeRenderTarget(mRHI.get(), "GBuffer_Albedo_Clip",   vk::Format::eR16G16B16A16Sfloat, std::nullopt, samplerInfo);
        mEmissiveBuffer = makeRenderTarget(mRHI.get(), "GBuffer_Emissive",      vk::Format::eR8Uint,             std::nullopt, samplerInfo);
        mParamsBuffer   = makeRenderTarget(mRHI.get(), "GBuffer_Params",        vk::Format::eR16G16Sfloat,       std::nullopt, samplerInfo);
        mDepthBuffer    = makeRenderTarget(mRHI.get(), "GBuffer_Depth",         vk::Format::eD32Sfloat);

        mScissor = getRenderAreaForAttachment(mWorldPosition.get());
        mViewport = vk::Viewport {
            0.0f, 0.0f,
            static_cast<float>(mScissor.extent.width), static_cast<float>(mScissor.extent.height),
            0.0f, 1.0f
        };

        mRenderPass = mRHI->createRenderPass({
            .renderArea       = mScissor,
            .colorAttachments = {
                makeAttachment(mWorldPosition),
                makeAttachment(mWorldNormal),
                makeAttachment(mMotionVectors),
                makeAttachment(mViewZ),
                makeAttachment(mAlbedoBuffer),
                makeAttachment(mEmissiveBuffer),
                makeAttachment(mParamsBuffer),
            },
            .depthAttachment  = makeAttachment(mDepthBuffer),
            .label            = "GBuffer_RenderPass",
        });

        const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .addDescriptorSetLayout(mInput.pTextureManager->getDescriptor()->getLayout())
            .setPushConstantRange<PushConstants>(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .setStateInfo(RHI::makeGraphicsStateInfo([&](RHI::GraphicsPipelineStateInfo& stateInfo)
            {
                stateInfo
                    .addDefaultAttachmentStates(7)
                    .addAttributeDescriptions<Vertex>(0, 0)
                    .addBindingDescriptions<Vertex>(0);
            }))
            .addShader({ Configuration::getShaderFilePath("GBuffer.vert.spv"), vk::ShaderStageFlagBits::eVertex })
            .addShader({ Configuration::getShaderFilePath("GBuffer.frag.spv"), vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mWorldPosition->getProperties().format)
            .addColorAttachmentFormat(mWorldNormal->getProperties().format)
            .addColorAttachmentFormat(mMotionVectors->getProperties().format)
            .addColorAttachmentFormat(mViewZ->getProperties().format)
            .addColorAttachmentFormat(mAlbedoBuffer->getProperties().format)
            .addColorAttachmentFormat(mEmissiveBuffer->getProperties().format)
            .addColorAttachmentFormat(mParamsBuffer->getProperties().format)
            .setDepthAttachmentFormat(mDepthBuffer->getProperties().format)
            .setDebugName("GBuffer_Pipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }
}
