#include "FXAAPass.hpp"

#include "VulkanRHI/Barrier.hpp"

FXAAPass::FXAAPass(const FXAA_Params& params)
: RenderPass({ params.resolution, params.rhi, "FXAA" })
, mInput(params.input)
{
    createResources();
    createPipeline();
}

UPtr<FXAAPass> FXAAPass::create(const FXAA_Params& params) noexcept
{
    return makeUnique<FXAAPass>(params);
}

void FXAAPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    const PushConstant pushConstant { 1.0f / static_cast<float>(mRenderResolution.width), 1.0f / static_cast<float>(mRenderResolution.height), 0.0f, 0.0f };

    RHI::Barrier()
        .addBarrier(mInput.input->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mOutput->getBarrier(RHI::ImageUsage::ColorAttachment))
        .insert(pCommandList);

    mRenderPass->execute(pCommandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) {
        mPipeline->bind(commandBuffer);
        mPipeline->bindDescriptorSet(commandBuffer, mDescriptor->getSet(0));
        mPipeline->pushConstants(commandBuffer, &pushConstant);
        commandBuffer.draw(3, 1, 0, 0);
    });
}

SPtr<RHI::Image> FXAAPass::getResult() const noexcept
{
    return mOutput;
}

void FXAAPass::createResources() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mOutput = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .debugName     = "FXAA_Output",
    });

    mDescriptor = mRHI->createDescriptor({
       .bindings = {
           { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
       },
       .setCount = 1,
       .debugName = "FXAA_Descriptor",
   });

    const auto descriptorWrite = RHI::DescriptorWrite()
        .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.input);
    mDescriptor->write(0, descriptorWrite);
}

void FXAAPass::createPipeline() noexcept
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
        .setPushConstantRange(vk::PushConstantRange().setOffset(0).setSize(sizeof(PushConstant)).setStageFlags(vk::ShaderStageFlagBits::eFragment))
        .addDescriptorSetLayout(mDescriptor->getLayout())
        .setStateInfo(RHI::GraphicsPipelineStateInfo()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentStates(1))
        .addShader({ "Resources/Shaders/bin/FXAA.vert.spv", vk::ShaderStageFlagBits::eVertex })
        .addShader({ "Resources/Shaders/bin/FXAA.frag.spv", vk::ShaderStageFlagBits::eFragment })
        .addColorAttachmentFormat(mOutput->getProperties().format)
        .setDebugName("FXAA_Pipeline");

    mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
}
