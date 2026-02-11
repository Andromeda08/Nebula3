#include "Voxel_GBufferPass.hpp"

#include "Scene/Scene.hpp"
#include "Scene/Voxel/GPUVoxelInstanceData.hpp"
#include "Scene/Voxel/VoxelScene.hpp"

Voxel_GBufferPass::Voxel_GBufferPass(const GBuffer_Params& params)
: RenderPass({ params.resolution, params.rhi, "Voxel_GBuffer" })
, mScene(params.pScene)
{
    createResources();
    createPipeline();
}

UPtr<Voxel_GBufferPass> Voxel_GBufferPass::create(const GBuffer_Params& params) noexcept
{
    return makeUnique<Voxel_GBufferPass>(params);
}

void Voxel_GBufferPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    pCommandList->beginLabel("Voxel_GBufferPass");

    setScissorViewport(pCommandList);

    // Barriers
    RHI::Barrier()
        .addBarrier(mPositionDepthBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mNormalBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mAlbedoBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mDepthBuffer->getBarrier(RHI::ImageUsage::DepthAttachment))
        .insert(pCommandList);

    // RenderPass
    mRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmdList) -> void {
        cmdList->bindPipeline(mPipeline.get());

        // TODO: RHI param instead of vk handle
        mPipeline->bindDescriptorSet(cmdList->getHandle(), mScene->getSceneDescriptor()->getSet(frameData.currentFrame));
        mPipeline->pushConstants(cmdList->getHandle(), &mScene->mParams);

        static std::vector<RHI2::DeviceSize> offsets = { 0, 0 };
        const std::vector vertexBuffers{ mScene->mVertexBuffer.get(), mScene->mInstanceBuffer.get() };
        cmdList->bindVertexBuffers(0, vertexBuffers, offsets);
        cmdList->bindIndexBuffer(mScene->mIndexBuffer.get(), 0, RHI2::IndexType::Uint32);
        cmdList->drawIndexed(mScene->mCube->indexCount(), mScene->mInstanceData.size(), 0, 0, 0);
    });

    pCommandList->endLabel();
}

const SPtr<RHI::Image>& Voxel_GBufferPass::getPosition() const noexcept
{
    return mPositionDepthBuffer;
}

const SPtr<RHI::Image>& Voxel_GBufferPass::getNormal() const noexcept
{
    return mNormalBuffer;
}

const SPtr<RHI::Image>& Voxel_GBufferPass::getAlbedo() const noexcept
{
    return mAlbedoBuffer;
}

void Voxel_GBufferPass::createResources() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mPositionDepthBuffer = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .debugName     = "GBuffer_PositionDepth",
    });
    mNormalBuffer = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .debugName     = "GBuffer_Normal",
    });
    mAlbedoBuffer = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .debugName     = "GBuffer_Albedo",
    });
    mDepthBuffer = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eD32Sfloat,
        .usageFlags    = eDepthStencilAttachment | eSampled | eTransferSrc | eTransferDst,
        .debugName     = "GBuffer_DepthBuffer"
    });
}

void Voxel_GBufferPass::createPipeline() noexcept
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
        .setPushConstantRange({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(VoxelSceneParams) })
        .addDescriptorSetLayout(mScene->getSceneDescriptor()->getLayout())
        .setStateInfo(RHI::GraphicsPipelineStateInfo()
            .configure([](RHI::GraphicsPipelineStateInfo& stateInfo) {
                stateInfo.addAttributeDescriptions<Vertex>(0, 0);
                stateInfo.addBindingDescriptions<Vertex>(0);
                stateInfo.addAttributeDescriptions<GPUVoxelInstanceData>(Vertex::getAttributeCount(), 1);
                stateInfo.addBindingDescriptions<GPUVoxelInstanceData>(1);
            })
            .addDefaultAttachmentStates(3))
        .addShader({ "Resources/Shaders/bin/VoxelGBuffer.vert.spv", vk::ShaderStageFlagBits::eVertex })
        .addShader({ "Resources/Shaders/bin/VoxelGBuffer.frag.spv", vk::ShaderStageFlagBits::eFragment })
        .addColorAttachmentFormat(mPositionDepthBuffer->getProperties().format)
        .addColorAttachmentFormat(mNormalBuffer->getProperties().format)
        .addColorAttachmentFormat(mAlbedoBuffer->getProperties().format)
        .setDepthAttachmentFormat(mDepthBuffer->getProperties().format)
        .setDebugName("Voxel-GBuffer-Pipeline");

    mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
}
