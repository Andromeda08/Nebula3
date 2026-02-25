#include "Indirect_GBufferPass.hpp"

#include "Scene/SceneV2.hpp"
#include "VulkanRHI/Barrier.hpp"

Indirect_GBufferPass::Indirect_GBufferPass(const Indirect_GBuffer_Params& params)
: RenderPass({ params.resolution, params.rhi, "Indirect_GBuffer" })
, mScene(params.pScene)
{
    createResources();
    createPipeline();
}

UPtr<Indirect_GBufferPass> Indirect_GBufferPass::create(const Indirect_GBuffer_Params& params) noexcept
{
    return makeUnique<Indirect_GBufferPass>(params);
}

void Indirect_GBufferPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    pCommandList->beginLabel("Indirect_GBufferPass");

    setScissorViewport(pCommandList);

    // Barriers
    RHI::Barrier()
        .addBarrier(mPositionDepthBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mNormalBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mAlbedoBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mDepthBuffer->getBarrier(RHI::ImageUsage::DepthAttachment))
        .insert(pCommandList);

    {
        const auto b1 = vk::BufferMemoryBarrier2()
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllTransfer)
            .setDstStageMask(vk::PipelineStageFlagBits2::eVertexShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderStorageRead)
            .setBuffer(mScene->mInstanceMapBuffer->getHandle())
            .setSize(VK_WHOLE_SIZE);
        const auto b2 = vk::BufferMemoryBarrier2()
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllTransfer)
            .setDstStageMask(vk::PipelineStageFlagBits2::eVertexShader | vk::PipelineStageFlagBits2::eDrawIndirect)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eIndirectCommandRead)
            .setBuffer(mScene->mDrawCmdBuffer->getHandle())
            .setSize(VK_WHOLE_SIZE);
        std::array barriers = { b1, b2 };
        const auto dependencyInfo = vk::DependencyInfo()
            .setBufferMemoryBarriers(barriers);
        pCommandList->getHandle().pipelineBarrier2(dependencyInfo);
    }

    // RenderPass
    mRenderPass->execute(pCommandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
        const PushConstants pc = {
            .instanceBufferAddress = mScene->mInstancePool->getBuffer()->getAddress(),
            .instanceMapAddress    = mScene->mInstanceMapBuffer->getAddress(),
        };

        mPipeline->bind(commandBuffer);
        mPipeline->bindDescriptorSets(commandBuffer, {
            mScene->getSceneDescriptor()->getSet(frameData.currentFrame),
            mScene->mTextureManager->getDescriptor()->getSet(0),
        });
        mPipeline->pushConstants(commandBuffer, &pc);

        static constexpr vk::DeviceSize offsets[1] = { 0 };
        const std::array vertexBuffers{ mScene->mGeometry->getVertexBuffer()->getHandle() };
        commandBuffer.bindVertexBuffers(0, 1, vertexBuffers.data(), offsets);
        commandBuffer.bindIndexBuffer(mScene->mGeometry->getIndexBuffer()->getHandle(), 0, vk::IndexType::eUint32);

        commandBuffer.drawIndexedIndirect(
            mScene->mDrawCmdBuffer->getHandle(),
            0, mScene->mDrawCount, sizeof(vk::DrawIndexedIndirectCommand));
    });

    pCommandList->endLabel();
}

const SPtr<RHI::Image>& Indirect_GBufferPass::getPosition() const noexcept
{
    return mPositionDepthBuffer;
}

const SPtr<RHI::Image>& Indirect_GBufferPass::getNormal() const noexcept
{
    return mNormalBuffer;
}

const SPtr<RHI::Image>& Indirect_GBufferPass::getAlbedo() const noexcept
{
    return mAlbedoBuffer;
}

void Indirect_GBufferPass::createResources() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mPositionDepthBuffer = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst | eStorage,
        .debugName     = "Indirect_GBuffer_PositionDepth",
    });
    mNormalBuffer = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst | eStorage,
        .debugName     = "Indirect_GBuffer_Normal",
    });
    mAlbedoBuffer = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst | eStorage,
        .debugName     = "Indirect_GBuffer_Albedo",
    });
    mDepthBuffer = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eD32Sfloat,
        .usageFlags    = eDepthStencilAttachment | eSampled | eTransferSrc | eTransferDst | eStorage,
        .debugName     = "Indirect_GBuffer_DepthBuffer"
    });
}

void Indirect_GBufferPass::createPipeline() noexcept
{
    mRenderPass = mRHI->createRenderPass({
        .renderArea = getRenderArea(),
        .colorAttachments = {
            RHI::Attachment {
                .image = mPositionDepthBuffer->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mPositionDepthBuffer->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
            },
            RHI::Attachment {
                .image = mNormalBuffer->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mNormalBuffer->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
            },
            RHI::Attachment {
                .image = mAlbedoBuffer->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mAlbedoBuffer->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
            }
        },
        .depthAttachment  = RHI::Attachment {
            .image = mDepthBuffer->getImage(),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setDepthStencil({1.0f, 0}))
                    .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                    .setImageView(mDepthBuffer->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore),
        },
        .label = "GBuffer_RenderPass",
    });

    const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
        .setPushConstantRange({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants) })
        .addDescriptorSetLayout(mScene->getSceneDescriptor()->getLayout())
        .addDescriptorSetLayout(mScene->mTextureManager->getDescriptor()->getLayout())
        .setStateInfo(RHI::GraphicsPipelineStateInfo()
            .configure([](RHI::GraphicsPipelineStateInfo& stateInfo) {
                stateInfo.addAttributeDescriptions<Vertex>(0, 0);
                stateInfo.addBindingDescriptions<Vertex>(0);
            })
            .addDefaultAttachmentStates(3))
        .addShader({ "Resources/Shaders/bin/IndirectDrawGBuffer.vert.spv", vk::ShaderStageFlagBits::eVertex })
        .addShader({ "Resources/Shaders/bin/VoxelGBuffer.frag.spv", vk::ShaderStageFlagBits::eFragment })
        .addColorAttachmentFormat(mPositionDepthBuffer->getProperties().format)
        .addColorAttachmentFormat(mNormalBuffer->getProperties().format)
        .addColorAttachmentFormat(mAlbedoBuffer->getProperties().format)
        .setDepthAttachmentFormat(mDepthBuffer->getProperties().format)
        .setDebugName("Indirect_GBuffer_Pipeline");

    mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
}
