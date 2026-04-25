#include "TonemapPass.hpp"

#include "VulkanRHI/Barrier.hpp"

TonemapPass::TonemapPass(const Tonemap_Params& params)
: RenderPass({ params.resolution, params.rhi, "Tonemap_Pass"})
, mInput(params.input)
{
    createResources();
    createPipeline();
}

UPtr<TonemapPass> TonemapPass::create(const Tonemap_Params& params) noexcept
{
    return makeUnique<TonemapPass>(params);
}

void TonemapPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    pCommandList->beginLabel("Tonemap_Pass");

    // Barriers
    RHI::Barrier()
        .addBarrier(mInput.color->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mOutput->getBarrier(RHI::ImageUsage::ColorAttachment))
        .insert(pCommandList);

    // RenderPass
    mRenderPass->execute(pCommandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
        mPipeline->bind(commandBuffer);
        mPipeline->pushConstants(commandBuffer, &mPushConstant);
        mPipeline->bindDescriptorSet(commandBuffer, mDescriptor->getSet());
        commandBuffer.draw(3, 1, 0, 0);
    });

    pCommandList->endLabel();
}

SPtr<RHI::Image> TonemapPass::getResult() const noexcept
{
    return mOutput;
}

void TonemapPass::createResources() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mOutput = mRHI->createImage({
        .extent        = mInput.color->getProperties().extent,
        .format        = mInput.color->getProperties().format,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .createSampler = true,
        .debugName     = "Tonemap_Output",
    });

    mDescriptor = mRHI->createDescriptor({
        .bindings = {
            { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
        },
        .setCount = 1,
        .debugName = "Tonemap_Descriptor",
    });

    const auto descriptorWrite = RHI::DescriptorWrite()
        .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.color);
    mDescriptor->write(0, descriptorWrite);
}

void TonemapPass::createPipeline() noexcept
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
        .label = "Tonemap_RenderPass",
    });

    const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
        .setPushConstantRange<PushConstant>(vk::ShaderStageFlagBits::eFragment)
        .addDescriptorSetLayout(mDescriptor->getLayout())
        .setStateInfo(RHI::GraphicsPipelineStateInfo()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentStates(1))
        .addShader({ Configuration::getShaderFilePath("FSQuad.vert.spv").string(), vk::ShaderStageFlagBits::eVertex })
        .addShader({ Configuration::getShaderFilePath("Tonemap.frag.spv").string(), vk::ShaderStageFlagBits::eFragment })
        .addColorAttachmentFormat(mOutput->getProperties().format)
        .setDebugName("Tonemap_Pipeline");

    mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
}
