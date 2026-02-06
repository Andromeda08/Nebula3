#include "SSAOPass.hpp"

#include <glm/glm.hpp>
#include "Core/Random.hpp"
#include "VulkanRHI/Barrier.hpp"

SSAOPass::SSAOPass(const SSAO_Params& params)
: RenderPass({ params.resolution, params.rhi, "SSAO" })
, mInput(params.input)
, mRunBlurPass(params.useBlur)
{
    createKernel();
    createNoiseTexture();

    createResources_SSAO();
    createResources_Blur();
}

UPtr<SSAOPass> SSAOPass::create(const SSAO_Params& params) noexcept
{
    return makeUnique<SSAOPass>(params);
}

void SSAOPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    pCommandList->beginLabel("Screen-Space AO");

    setScissorViewport(pCommandList);

    execute_SSAO(pCommandList, frameData);

    if (mRunBlurPass)
    {
        execute_Blur(pCommandList, frameData);
    }

    pCommandList->endLabel();
}

const SPtr<RHI::Image>& SSAOPass::getResult() const noexcept
{
    return mRunBlurPass ? mBlur_Result : mSSAO_Result;
}

const SPtr<RHI::Image>& SSAOPass::getSSAOResult() const noexcept
{
    return mSSAO_Result;
}

const SPtr<RHI::Image>& SSAOPass::getBlurredResult() const noexcept
{
    return mBlur_Result;
}

void SSAOPass::createKernel() noexcept
{
    std::vector<glm::vec4> kernel(sKernelSize);
    for (auto i = 0; i < sKernelSize; i++)
    {
        glm::vec3 sample = {
            Random::get(-1.0f, 1.0f),
            Random::get(-1.0f, 1.0f),
            Random::unit(),
        };
        sample = glm::normalize(sample);
        sample *= Random::unit();
        float scale = static_cast<float>(i) / static_cast<float>(sKernelSize);
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        kernel[i] = glm::vec4(sample * scale, 0.0f);
    }

    mSSAO_Kernel = mRHI->createBuffer({
        .size  = sKernelSize * sizeof(glm::vec4),
        .type  = RHI::BufferType::Uniform,
        .label = std::format("SSAO_Kernel[n={}]", sKernelSize)
    });
    mSSAO_Kernel->setData(kernel.data(), sKernelSize * sizeof(glm::vec4));
}

void SSAOPass::createNoiseTexture() noexcept
{
    std::vector<glm::vec4> noise(sNoiseSize * sNoiseSize);
    for (auto i = 0; i < noise.size(); i++)
    {
        noise[i] = glm::vec4 {
            Random::get(-1.0f, 1.0f),
            Random::get(-1.0f, 1.0f),
            0.0f,
            0.0f,
        };
    }

    const auto noiseStaging = mRHI->createBuffer({ noise.size() * sizeof(glm::vec4), RHI::BufferType::Staging, "SSAO-Noise-Staging" });
    noiseStaging->setData(noise.data(), noise.size() * sizeof(glm::vec4));

    using enum vk::ImageUsageFlagBits;
    mSSAO_Noise = mRHI->createImage({
        .extent = { sNoiseSize, sNoiseSize },
        .format = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags = eSampled | eTransferDst,
        .createSampler = true,
        .debugName = std::format("SSAO_Noise[d={}]", sNoiseSize),
    });

    mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void {
        RHI::Barrier().addImageBarrier({
             .dstUsage = RHI::ImageUsage::TransferDst,
             .image = mSSAO_Noise,
         }).insert(pCommandList);

        pCommandList->copyBufferToImage({
           .pSrcBuffer = noiseStaging.get(),
           .pDstImage  = mSSAO_Noise.get(),
       });

        RHI::Barrier().addImageBarrier({
             .dstUsage = RHI::ImageUsage::ShaderReadOnly,
             .image = mSSAO_Noise,
         }).insert(pCommandList);
    });
}

void SSAOPass::createResources_SSAO() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mSSAO_Result = mRHI->createImage({
        .extent        = { mRenderResolution.width, mRenderResolution.height },
        .format        = vk::Format::eR32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .createSampler = true,
        .debugName     = "SSAO_Result",
    });

    mSSAO_Descriptor = mRHI->createDescriptor({
        .bindings = {
            { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment },
            { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
            { 2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
            { 3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
        },
        .setCount = 1,
        .debugName = "SSAO_Descriptor",
    });

    const auto descriptorWrite = RHI::DescriptorWrite()
        .writeUniformBuffer(0, mSSAO_Kernel)
        .writeCombinedImageSampler(1, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mSSAO_Noise)
        .writeCombinedImageSampler(2, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.positionBuffer)
        .writeCombinedImageSampler(3, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.normalBuffer);
    mSSAO_Descriptor->write(0, descriptorWrite);

    mSSAO_RenderPass = mRHI->createRenderPass({
        .renderArea = getRenderArea(),
        .colorAttachments = {
            RHI::Attachment {
                .image = mSSAO_Result->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mSSAO_Result->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
            },
        },
        .label = "SSAO_RenderPass",
    });

    const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
        .addDescriptorSetLayout(mInput.sceneDescriptor->getLayout())
        .addDescriptorSetLayout(mSSAO_Descriptor->getLayout())
        .setStateInfo(RHI::GraphicsPipelineStateInfo()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentStates(1))
        .addShader({ "Resources/Shaders/bin/FSQuad.vert.spv", vk::ShaderStageFlagBits::eVertex })
        .addShader({ "Resources/Shaders/bin/SSAO.frag.spv", vk::ShaderStageFlagBits::eFragment })
        .addColorAttachmentFormat(mSSAO_Result->getProperties().format)
        .setDebugName("SSAO_Pipeline");

    mSSAO_Pipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
}

void SSAOPass::createResources_Blur() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mBlur_Result = mRHI->createImage({
        .extent        = { mRenderResolution.width, mRenderResolution.height },
        .format        = vk::Format::eR32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .createSampler = true,
        .debugName     = "SSAO_Blur_Result",
    });

    mBlur_Descriptor = mRHI->createDescriptor({
        .bindings = {
            { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
        },
        .setCount = 1,
        .debugName = "SSAO_Blur_Descriptor",
    });

    const auto descriptorWrite = RHI::DescriptorWrite()
        .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mSSAO_Result);
    mBlur_Descriptor->write(0, descriptorWrite);

    mBlur_RenderPass = mRHI->createRenderPass({
        .renderArea = getRenderArea(),
        .colorAttachments = {
            RHI::Attachment {
                .image = mBlur_Result->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mBlur_Result->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
            },
        },
        .label = "SSAO_Blur_RenderPass",
    });

    const auto ssao_blur_pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
        .addDescriptorSetLayout(mBlur_Descriptor->getLayout())
        .setStateInfo(RHI::GraphicsPipelineStateInfo()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentStates(1))
        .addShader({ "Resources/Shaders/bin/FSQuad.vert.spv", vk::ShaderStageFlagBits::eVertex })
        .addShader({ "Resources/Shaders/bin/SSAO_Blur.frag.spv", vk::ShaderStageFlagBits::eFragment })
        .addColorAttachmentFormat(mBlur_Result->getProperties().format)
        .setDebugName("SSAO_Blur_Pipeline");

    mBlur_Pipeline = mRHI->createGraphicsPipeline(ssao_blur_pipelineCreateInfo);
}

void SSAOPass::execute_SSAO(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
{
    pCommandList->beginLabel("SSAO_Main_Pass");
    RHI::Barrier()
        .addBarrier(mSSAO_Result->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mSSAO_Noise->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mInput.positionBuffer->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .addBarrier(mInput.normalBuffer->getBarrier(RHI::ImageUsage::ShaderReadOnly))
        .insert(pCommandList);

    mSSAO_RenderPass->execute(pCommandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
        mSSAO_Pipeline->bind(commandBuffer);
        mSSAO_Pipeline->bindDescriptorSets(commandBuffer, {
            mInput.sceneDescriptor->getSet(frameData.currentFrame),
            mSSAO_Descriptor->getSet(0),
        });
        commandBuffer.draw(3, 1, 0, 0);
    });
    pCommandList->endLabel();
}

void SSAOPass::execute_Blur(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
{
    pCommandList->beginLabel("SSAO_Blur_Pass");
    mBlur_RenderPass->execute(pCommandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
        mBlur_Pipeline->bind(commandBuffer);
        mBlur_Pipeline->bindDescriptorSet(commandBuffer, mBlur_Descriptor->getSet(0));
        commandBuffer.draw(3, 1, 0, 0);
    });
    pCommandList->endLabel();
}
