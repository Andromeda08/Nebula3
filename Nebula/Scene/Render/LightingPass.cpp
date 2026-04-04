#include "LightingPass.hpp"

#include "VulkanRHI/Barrier.hpp"

LightingPass::LightingPass(const Lighting_Params& params)
: RenderPass({ params.resolution, params.rhi, "Lighting" })
, mInput(params.input)
{
    createResources();
    createPipeline();
}

UPtr<LightingPass> LightingPass::create(const Lighting_Params& params) noexcept
{
    return makeUnique<LightingPass>(params);
}

void LightingPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    pCommandList->beginLabel("Lighting_Pass");

    auto barriers = RHI::Barrier()
        .addBarrier(mOutput->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mInput.position->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mInput.normal->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mInput.albedo->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mInput.ssao->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mInput.cubeMap->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mInput.skyData->getBarrier(RHI::BufferUsage::Compute, RHI::BufferUsage::Fragment));

    if (mRHI->getRaytracingSupport())
    {
        barriers.addBarrier(mInput.tlasManager->getBackingBuffer()->getBarrier(RHI::BufferUsage::AS_BuildUpdate, RHI::BufferUsage::AS_Traverse));
    }

    barriers.insert(pCommandList);

    mRenderPass->execute(pCommandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
        mPipeline->bind(commandBuffer);
        mPipeline->pushConstants(commandBuffer, &mPushConstants);
        mPipeline->bindDescriptorSets(commandBuffer, {
            mInput.sceneDescriptor->getSet(frameData.currentFrame),
            mDescriptor->getSet(0),
        });
        commandBuffer.draw(3, 1, 0, 0);
    });
    pCommandList->endLabel();
}

SPtr<RHI::Image> LightingPass::getResult() const noexcept
{
    return mOutput;
}

void LightingPass::setShadowMode(const int32_t mode) noexcept
{
    mPushConstants.shadowMode = mode;
}

void LightingPass::createResources() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mOutput = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .createSampler = true,
        .debugName     = "Lighting_Output",
    });

    mDescriptor = mRHI->createDescriptor({
       .bindings = {
           { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
           { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
           { 2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
           { 3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
           { 4, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
           { 5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
       },
       .setCount = 1,
       .debugName = "Lighting_Descriptor",
   });

    const auto descriptorWrite = RHI::DescriptorWrite()
        .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.position)
        .writeCombinedImageSampler(1, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.normal)
        .writeCombinedImageSampler(2, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.albedo)
        .writeCombinedImageSampler(3, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.ssao)
        .writeCombinedImageSampler(4, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.cubeMap)
        .writeStorageBuffer(5, mInput.skyData);
    mDescriptor->write(0, descriptorWrite);
}

void LightingPass::createPipeline() noexcept
{
    mRenderPass = mRHI->createRenderPass({
        .renderArea = getRenderArea(),
        .colorAttachments = {
            RHI::Attachment {
                .image = mOutput->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mOutput->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
            },
        },
        .label = "Lighting_RenderPass",
    });

    const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
        .setPushConstantRange({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstants) })
        .addDescriptorSetLayout(mInput.sceneDescriptor->getLayout())
        .addDescriptorSetLayout(mDescriptor->getLayout())
        .setStateInfo(RHI::GraphicsPipelineStateInfo()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentStates(1))
        .addShader({ Configuration::getShaderFilePath("FSQuad.vert.spv").string(), vk::ShaderStageFlagBits::eVertex })
        .addShader({ Configuration::getShaderFilePath("Lighting.frag.spv").string(), vk::ShaderStageFlagBits::eFragment })
        .addColorAttachmentFormat(mOutput->getProperties().format)
        .setDebugName("Lighting_Pipeline");

    mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
}
