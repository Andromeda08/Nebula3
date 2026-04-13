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
    const auto& drawCommandsBuffer = mScene->mDrawCmdBuffer[frameData.currentFrame];
    const auto& instanceMapBuffer = mScene->mInstanceMapBuffer[frameData.currentFrame];

    setScissorViewport(pCommandList);

    // Barriers
    RHI::Barrier()
        .addBarrier(mPositionDepthBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mNormalBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mAlbedoBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mEmissiveBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mDepthBuffer->getBarrier(RHI::ImageUsage::DepthAttachment))
        .addBarrier(instanceMapBuffer->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::StorageRead ))
        .addBarrier(drawCommandsBuffer->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::DrawIndirect))
        .insert(pCommandList);

    // RenderPass
    mRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
        const PushConstants pc = {
            .instanceBufferAddress = mScene->mInstancePool->getBuffer()->getAddress(),
            .instanceMapAddress    = instanceMapBuffer->getAddress(),
        };

        mPipeline->bind(cmd);
        mPipeline->bindDescriptorSets(cmd, {
            mScene->getSceneDescriptor()->getSet(frameData.currentFrame),
            mScene->mTextureManager->getDescriptor()->getSet(0),
        });
        mPipeline->pushConstants(cmd, &pc);

        static constexpr vk::DeviceSize offsets[1] = { 0 };
        const auto [ vertexBuffer, indexBuffer, _ ] = mScene->mGeometry->getBuffers();

        const std::array vertexBuffers { vertexBuffer->getHandle() };
        cmd->getHandle().bindVertexBuffers(0, 1, vertexBuffers.data(), offsets);
        cmd->getHandle().bindIndexBuffer(indexBuffer->getHandle(), 0, vk::IndexType::eUint32);

        cmd->getHandle().drawIndexedIndirect(
            drawCommandsBuffer->getHandle(),
            0, mScene->mDrawCount, sizeof(vk::DrawIndexedIndirectCommand));
    });

    pCommandList->endLabel();
}

SPtr<RHI::Image> Indirect_GBufferPass::getResult() const noexcept
{
    return mAlbedoBuffer;
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

const SPtr<RHI::Image>& Indirect_GBufferPass::getDepth() const noexcept
{
    return mDepthBuffer;
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
    mEmissiveBuffer = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst | eStorage,
        .debugName     = "Indirect_GBuffer_Emissive",
    });
    mMotionVectors = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst | eStorage,
        .debugName     = "Indirect_GBuffer_MotionVectors",
    });
    mDepthBuffer = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eD32Sfloat,
        .usageFlags    = eDepthStencilAttachment | eSampled | eTransferSrc | eTransferDst,
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
            },
            RHI::Attachment {
                .image = mEmissiveBuffer->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mEmissiveBuffer->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
            },
            RHI::Attachment {
                .image = mMotionVectors->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mMotionVectors->getImageView())
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
        .addShader({ Configuration::getShaderFilePath("IndirectDrawGBuffer.vert.spv").string(), vk::ShaderStageFlagBits::eVertex })
        .addShader({ Configuration::getShaderFilePath("IndirectDrawGBuffer.frag.spv").string(), vk::ShaderStageFlagBits::eFragment })
        .addColorAttachmentFormat(mPositionDepthBuffer->getProperties().format)
        .addColorAttachmentFormat(mNormalBuffer->getProperties().format)
        .addColorAttachmentFormat(mAlbedoBuffer->getProperties().format)
        .addColorAttachmentFormat(mEmissiveBuffer->getProperties().format)
        .addColorAttachmentFormat(mMotionVectors->getProperties().format)
        .setDepthAttachmentFormat(mDepthBuffer->getProperties().format)
        .setDebugName("Indirect_GBuffer_Pipeline");

    mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
}
