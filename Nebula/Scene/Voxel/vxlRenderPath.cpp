#include "vxlRenderPath.hpp"

#include "Scene/Voxel/VoxelScene.hpp"

vxlRenderPath::vxlRenderPath(const SPtr<RHI::VulkanRHI>& rhi, VoxelScene* pScene)
: mScene(pScene)
, mRHI(rhi)
{
    mRenderExtent     = mRHI->getSwapchain()->getProperties().extent;
    mRenderResolution = { mRenderExtent.width, mRenderExtent.height };

    // Create Passes
    resources_GBufferPass();
    create_GBufferPass();
}

void vxlRenderPath::execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept
{
    commandList->beginLabel("vxlRenderPath");

    execute_GBufferPass(commandList, frameData);

    // Blit final image to swapchain
    execute_BlitToSwapchain(mAlbedoBuffer.get(), commandList, frameData);

    commandList->endLabel();
}

vk::Rect2D vxlRenderPath::getRenderArea() const noexcept
{
    return vk::Rect2D()
        .setExtent({ mRenderResolution.width, mRenderResolution.height })
        .setOffset({ 0, 0 });
}

void vxlRenderPath::resources_GBufferPass() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mPositionBuffer = mRHI->createImage({
        .extent        = mRenderExtent,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .debugName     = "vxlPositionBuffer",
    });
    mNormalBuffer = mRHI->createImage({
        .extent        = mRenderExtent,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .debugName     = "vxlNormalBuffer",
    });
    mAlbedoBuffer = mRHI->createImage({
        .extent        = mRenderExtent,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .debugName     = "vxlAlbedoBuffer",
    });
    mDepthImage = mRHI->createImage({
        .extent        = mRenderExtent,
        .format        = vk::Format::eD32Sfloat,
        .usageFlags    = eDepthStencilAttachment | eSampled | eTransferSrc | eTransferDst,
        .debugName     = "vxlDepthImage"
    });
}

void vxlRenderPath::create_GBufferPass() noexcept
{
    mGBufferPass.renderPass = mRHI->createRenderPass({
        .renderArea = getRenderArea(),
        .colorAttachments = {
            RHI::Attachment {
                .image = mPositionBuffer->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mPositionBuffer->getImageView())
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
            .image = mDepthImage->getImage(),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setDepthStencil({1.0f, 0}))
                    .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                    .setImageView(mDepthImage->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore),
        },
        .label = "VoxelForward-Pass",
    });

    const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
        .setPushConstantRange({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(VoxelSceneParams) })
        .addDescriptorSetLayout(mScene->mSceneDescriptor->getLayout())
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
        .addColorAttachmentFormat(mPositionBuffer->getProperties().format)
        .addColorAttachmentFormat(mNormalBuffer->getProperties().format)
        .addColorAttachmentFormat(mAlbedoBuffer->getProperties().format)
        .setDepthAttachmentFormat(mDepthImage->getProperties().format)
        .setDebugName("Voxel-GBuffer-Pipeline");

    mGBufferPass.pipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
}

void vxlRenderPath::execute_GBufferPass(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept
{
    const auto& [pipeline, renderPass] = mGBufferPass;
    commandList->beginLabel("vxlGBufferPass");
    // Barriers
    const auto barrier = RHI::Barrier()
        .addBarrier(mPositionBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mNormalBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mAlbedoBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mDepthImage->getBarrier(RHI::ImageUsage::DepthAttachment));
    barrier.insert(commandList);

    // RenderPass
    renderPass->execute(commandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
        pipeline->bind(commandBuffer);
        pipeline->bindDescriptorSet(commandBuffer, mScene->mSceneDescriptor->getSet(frameData.currentFrame));
        pipeline->pushConstants(commandBuffer, &mScene->mParams);

        static constexpr vk::DeviceSize offsets[2] = { 0, 0 };
        const std::array vertexBuffers{ mScene->mVertexBuffer->getHandle(), mScene->mInstanceBuffer->getHandle() };
        commandBuffer.bindVertexBuffers(0, 2, vertexBuffers.data(), offsets);
        commandBuffer.bindIndexBuffer(mScene->mIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(mScene->mCube->indexCount(), mScene->mInstanceData.size(), 0, 0, 0);
    });
    commandList->endLabel();
}

void vxlRenderPath::execute_BlitToSwapchain(RHI::Image* pFinalImage, const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept
{
    commandList->beginLabel("vxlFinalImageBlit");
    // Barriers
    const auto barrier = RHI::Barrier()
        .addBarrier(pFinalImage->getBarrier(RHI::ImageUsage::TransferSrc))
        .addBarrier(mRHI->getSwapchain()->getBarrier(frameData.acquiredIndex, RHI::ImageUsage::TransferDst));
    barrier.insert(commandList);

    // Blit
    const auto srcExtent = pFinalImage->getProperties().extent;
    const auto dstExtent = mRHI->getSwapchain()->getProperties().extent;
    const auto region    = vk::ImageBlit2()
        .setSrcOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D { static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), 1 }
        })
        .setSrcSubresource(pFinalImage->getProperties().getSubresourceLayers())
        .setDstOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D { static_cast<int32_t>(dstExtent.width), static_cast<int32_t>(dstExtent.height), 1 }
        })
        .setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });

    const auto blit = vk::BlitImageInfo2()
        .setSrcImage(pFinalImage->getImage())
        .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setDstImage(mRHI->getSwapchain()->getImage(frameData.acquiredIndex))
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setFilter(vk::Filter::eLinear)
        .setRegions(region);

    commandList->getHandle().blitImage2(blit);
    commandList->endLabel();
}
