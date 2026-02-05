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

    resources_SSAOPass();
    create_SSAOPass();
}

void vxlRenderPath::execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept
{
    commandList->beginLabel("vxlRenderPath");

    const auto viewport = vk::Viewport()
        //.setX(0.0f).setY(mRenderExtent.height)
        .setX(0.0f).setY(0)
        .setWidth(mRenderExtent.width).setHeight(mRenderExtent.height)
        .setMinDepth(0.0f).setMaxDepth(1.0f);
    const auto scissor = vk::Rect2D()
        .setExtent(mRenderExtent)
        .setOffset({ 0, 0 });

    commandList->getHandle().setViewport(0, viewport);
    commandList->getHandle().setScissor(0, scissor);

    execute_GBufferPass(commandList, frameData);

    execute_SSAOPass(commandList, frameData);

    // Blit final image to swapchain
    execute_BlitToSwapchain(mSSAOBuffer.get(), commandList, frameData);

    commandList->endLabel();
}

vk::Rect2D vxlRenderPath::getRenderArea() const noexcept
{
    return vk::Rect2D()
        .setExtent({ mRenderResolution.width, mRenderResolution.height })
        .setOffset({ 0, 0 });
}

// G-Buffer Pass
// ========================================
#pragma region "G-Buffer Pass"

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
        .label = "Voxel-GBuffer-Pass",
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

#pragma endregion

void vxlRenderPath::resources_SSAOPass() noexcept
{
    using enum vk::ImageUsageFlagBits;

    // Kernel generation
    std::vector<glm::vec4> kernel(sSSAOKernelSize);
    for (auto i = 0; i < sSSAOKernelSize; i++)
    {
        glm::vec3 sample = {
            Random::unit() * 2.0f / 1.0f,
            Random::unit() * 2.0f / 1.0f,
            Random::unit(),
        };
        sample = glm::normalize(sample);
        sample *= Random::unit();
        float scale = static_cast<float>(i) / static_cast<float>(sSSAOKernelSize);
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        kernel[i] = glm::vec4(sample * scale, 0.0f);
    }

    mSSAOKernel = mRHI->createBuffer({ sSSAOKernelSize * sizeof(glm::vec4), RHI::BufferType::Uniform, "SSAO-Kernel" });
    mSSAOKernel->setData(kernel.data(), sSSAOKernelSize * sizeof(glm::vec4));

    // Noise texture generation
    std::vector<glm::vec4> noise(sSSAONoiseSize * sSSAONoiseSize);
    for (auto i = 0; i < noise.size(); i++)
    {
        noise[i] = glm::vec4 {
            Random::unit() * 2.0f - 1.0f,
            Random::unit() * 2.0f - 1.0f,
            0.0f,
            0.0f,
        };
    }

    const auto noiseStaging = mRHI->createBuffer({ noise.size() * sizeof(glm::vec4), RHI::BufferType::Staging, "SSAO-Noise-Staging" });
    noiseStaging->setData(noise.data(), noise.size() * sizeof(glm::vec4));

    mSSAONoise = mRHI->createImage({
        .extent = { sSSAONoiseSize, sSSAONoiseSize },
        .format = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .createSampler = true,
        .debugName = "vxlSSAO_Noise",
    });

    mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void {
        RHI::Barrier().addImageBarrier({
             .dstUsage = RHI::ImageUsage::TransferDst,
             .image = mSSAONoise,
         }).insert(pCommandList);

        pCommandList->copyBufferToImage({
           .pSrcBuffer = noiseStaging.get(),
           .pDstImage  = mSSAONoise.get(),
       });

        RHI::Barrier().addImageBarrier({
             .dstUsage = RHI::ImageUsage::ShaderReadOnly,
             .image = mSSAONoise,
         }).insert(pCommandList);
    });

    // SSAO Render Targets
    mSSAOBuffer = mRHI->createImage({
        //.extent        = { mRenderExtent.width / 2, mRenderExtent.height / 2 },
        .extent        = mRenderExtent,
        .format        = vk::Format::eR32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .debugName     = "vxlSSAO_Buffer",
    });
    mSSAO_BlurBuffer = mRHI->createImage({
        .extent        = mRenderExtent,
        .format        = vk::Format::eR32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .debugName     = "vxlSSAO_BlurBuffer",
    });

    // Descriptor Set
    mSSAODescriptor = mRHI->createDescriptor({
            .bindings = {
                vk::DescriptorSetLayoutBinding { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment },
                vk::DescriptorSetLayoutBinding { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                vk::DescriptorSetLayoutBinding { 2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                vk::DescriptorSetLayoutBinding { 3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
            },
            .setCount = 1,
            .debugName = "vxlSSAO_Descriptor",
        });

    const auto descriptorWrite = RHI::DescriptorWrite()
        .writeUniformBuffer(0, mSSAOKernel)
        .writeCombinedImageSampler(1, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mSSAONoise)
        .writeCombinedImageSampler(2, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mPositionBuffer)
        .writeCombinedImageSampler(3, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mNormalBuffer);
    mSSAODescriptor->write(0, descriptorWrite);
}

void vxlRenderPath::create_SSAOPass() noexcept
{
    mSSAOPass.renderPass = mRHI->createRenderPass({
        .renderArea = getRenderArea(),
        .colorAttachments = {
            RHI::Attachment {
                .image = mSSAOBuffer->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mSSAOBuffer->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
            },
        },
        .label = "vxlSSAO-Pass",
    });

    const auto ssao_pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
        .addDescriptorSetLayout(mScene->mSceneDescriptor->getLayout())
        .addDescriptorSetLayout(mSSAODescriptor->getLayout())
        .setStateInfo(RHI::GraphicsPipelineStateInfo()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentStates(1))
        .addShader({ "Resources/Shaders/bin/FSQuad.vert.spv", vk::ShaderStageFlagBits::eVertex })
        .addShader({ "Resources/Shaders/bin/SSAO.frag.spv", vk::ShaderStageFlagBits::eFragment })
        .addColorAttachmentFormat(mSSAOBuffer->getProperties().format)
        .setDebugName("Voxel-SSAO-Pipeline");

    mSSAOPass.pipeline = mRHI->createGraphicsPipeline(ssao_pipelineCreateInfo);

}

void vxlRenderPath::execute_SSAOPass(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept
{
    // const auto viewport = vk::Viewport()
    //     .setX(0.0f)
    //     .setY(0.0f)
    //     .setWidth(static_cast<float>(mSSAOBuffer->getProperties().extent.width))
    //     .setHeight(static_cast<float>(mSSAOBuffer->getProperties().extent.height))
    //     .setMinDepth(0.0f)
    //     .setMaxDepth(1.0f);
    //
    // commandList->getHandle().setViewport(0, viewport);
    //
    // const auto scissor = vk::Rect2D { {0, 0}, mRenderExtent };
    // commandList->getHandle().setScissor(0, scissor);

    const auto& [pipeline, renderPass] = mSSAOPass;
    commandList->beginLabel("vxlSSAOPass");
    // Barriers
    const auto barrier = RHI::Barrier()
        .addBarrier(mSSAOBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mSSAONoise->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mPositionBuffer->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mNormalBuffer->getBarrier(RHI::ImageUsage::ShaderReadOnly));
    barrier.insert(commandList);

    // RenderPass
    renderPass->execute(commandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
        pipeline->bind(commandBuffer);
        pipeline->bindDescriptorSets(commandBuffer, {
            mScene->mSceneDescriptor->getSet(frameData.currentFrame),
            mSSAODescriptor->getSet(0),
        });

        commandBuffer.draw(3, 1, 0, 0);
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
