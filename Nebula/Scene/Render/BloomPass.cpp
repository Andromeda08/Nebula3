#include "BloomPass.hpp"

#include "VulkanRHI/Barrier.hpp"

BloomPass::BloomPass(const Bloom_Params& params)
: RenderPass({ params.resolution, params.rhi, "Bloom" })
, mInput(params.input)
{
    createResources();
    createPipeline();
}

void BloomPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    pCommandList->beginLabel("Bloom");

    RHI::Barrier()
        .addBarrier(mOutput->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mBloomHorizontal->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mBloomVertical->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mInput.emissive->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mInput.lighting->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .insert(pCommandList);

    // Horizontal
    pCommandList->beginLabel("Bloom_Horizontal");
    mBloomRenderPass->setColorAttachment(0, RHI::Attachment {
        .image = mBloomHorizontal->getImage(),
        .attachmentInfo = vk::RenderingAttachmentInfo()
            .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setImageView(mBloomHorizontal->getImageView())
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
    });
    mBloomRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
        constexpr PushConstants pcs { .direction = glm::vec2(1.0f, 0.0f) };
        mBloomPipeline->bind(cmd);
        mBloomPipeline->pushConstants(cmd, &pcs);
        mBloomPipeline->bindDescriptorSet(cmd, mBloomDescriptorH->getSet(0));
        cmd->getHandle().draw(3, 1, 0, 0);
    });

    RHI::Barrier()
         .addBarrier(mBloomHorizontal->getBarrier(RHI::ImageUsage::ShaderReadOnly))
         .addBarrier(mBloomVertical->getBarrier(RHI::ImageUsage::ColorAttachment))
         .insert(pCommandList);

    pCommandList->endLabel();

    // Vertical
    pCommandList->beginLabel("Bloom_Vertical");
    mBloomRenderPass->setColorAttachment(0, RHI::Attachment {
        .image = mBloomVertical->getImage(),
        .attachmentInfo = vk::RenderingAttachmentInfo()
            .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setImageView(mBloomVertical->getImageView())
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
    });
    mBloomRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
        constexpr PushConstants pcs { .direction = glm::vec2(0.0f, 1.0f) };
        mBloomPipeline->bind(cmd);
        mBloomPipeline->pushConstants(cmd, &pcs);
        mBloomPipeline->bindDescriptorSet(cmd, mBloomDescriptorV->getSet(0));
        cmd->getHandle().draw(3, 1, 0, 0);
    });

    RHI::Barrier()
     .addBarrier(mBloomVertical->getBarrier(RHI::ImageUsage::ShaderReadOnly))
     .insert(pCommandList);

    pCommandList->endLabel();

    // Composite
    pCommandList->beginLabel("Bloom_Composite");
    mCompositeRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
        mCompositePipeline->bind(cmd);
        mCompositePipeline->bindDescriptorSet(cmd, mCompositeDescriptor->getSet(0));
        cmd->getHandle().draw(3, 1, 0, 0);
    });
    pCommandList->endLabel();

    pCommandList->endLabel();
}

SPtr<RHI::Image> BloomPass::getResult() const noexcept
{
    return mOutput;
}

void BloomPass::createResources() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mBloomHorizontal = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .createSampler = true,
        .debugName     = "Bloom_Output",
    });
    mBloomVertical = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .createSampler = true,
        .debugName     = "Bloom_Output",
    });
    mOutput = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .createSampler = true,
        .debugName     = "Bloom_Composite_Output",
    });

    /* mBloomDescriptorH */ {
        mBloomDescriptorH = mRHI->createDescriptor({
           .bindings  = {{ 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }},
           .setCount  = 1,
           .debugName = "Bloom_Descriptor",
        });

        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.emissive);
        mBloomDescriptorH->write(0, descriptorWrite);
    }
    /* mBloomDescriptorV */ {
        mBloomDescriptorV = mRHI->createDescriptor({
           .bindings  = {{ 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }},
           .setCount  = 1,
           .debugName = "Bloom_Descriptor",
        });

        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mBloomHorizontal);
        mBloomDescriptorV->write(0, descriptorWrite);
    }
    /* mCompositeDescriptor */ {
        mCompositeDescriptor = mRHI->createDescriptor({
           .bindings  = {
               { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
               { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
           },
           .setCount  = 1,
           .debugName = "Bloom_Composite_Descriptor",
        });

        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.lighting)
            .writeCombinedImageSampler(1, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mBloomVertical);
        mCompositeDescriptor->write(0, descriptorWrite);
    }
}

void BloomPass::createPipeline() noexcept
{
    /* Bloom Blur */ {
        mBloomRenderPass = mRHI->createRenderPass({
            .renderArea = getRenderArea(),
            .colorAttachments = {
                RHI::Attachment {
                    .image = mBloomHorizontal->getImage(),
                    .attachmentInfo = vk::RenderingAttachmentInfo()
                        .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                        .setImageView(mBloomHorizontal->getImageView())
                        .setLoadOp(vk::AttachmentLoadOp::eClear)
                        .setStoreOp(vk::AttachmentStoreOp::eStore)
                },
            },
            .label = "Bloom_RenderPass",
        });

        const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .setPushConstantRange({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstants) })
            .addDescriptorSetLayout(mBloomDescriptorH->getLayout())
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .setCullMode(vk::CullModeFlagBits::eNone)
                .addDefaultAttachmentStates(1))
            .addShader({ Configuration::getShaderFilePath("FSQuad.vert.spv").string(), vk::ShaderStageFlagBits::eVertex })
            .addShader({ Configuration::getShaderFilePath("BloomBlur.frag.spv").string(), vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mBloomHorizontal->getProperties().format)
            .setDebugName("Bloom_Pipeline");

        mBloomPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }

    /* Bloom Composite */ {
        mCompositeRenderPass = mRHI->createRenderPass({
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
            .label = "Bloom_RenderPass",
        });

        const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .addDescriptorSetLayout(mCompositeDescriptor->getLayout())
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .setCullMode(vk::CullModeFlagBits::eNone)
                .addDefaultAttachmentStates(1))
            .addShader({ Configuration::getShaderFilePath("FSQuad.vert.spv").string(), vk::ShaderStageFlagBits::eVertex })
            .addShader({ Configuration::getShaderFilePath("BloomComposite.frag.spv").string(), vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mOutput->getProperties().format)
            .setDebugName("Bloom_Pipeline");

        mCompositePipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }
}
