#include "SSAOPass.hpp"

#include "Templates.hpp"
#include "Core/Random.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace nbl
{
    SSAOPass::SSAOPass(const SPtr<RHI::VulkanRHI>& rhi)
    : mRHI(rhi)
    {
        createKernel();
        createNoiseTexture();

        createResources_SSAO();
        createResources_Blur();
    }

    void SSAOPass::execute(const Input& input, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        pCommandList->beginLabel("SSAO");
        execute_SSAO(input, pCommandList, frameData);
        execute_Blur(pCommandList, frameData);
        pCommandList->endLabel();
    }

    const SPtr<RHI::Image>& SSAOPass::getResult(const uint32_t currentFrame) const noexcept
    {
        return mBlur_Result[currentFrame];
    }

    const PerFrameArray<SPtr<RHI::Image>>& SSAOPass::getResults() const noexcept
    {
        return mBlur_Result;
    }

    void SSAOPass::createKernel() noexcept
    {
        std::array<glm::vec4, sKernelSize> kernel;
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
        std::array<glm::vec4, sNoiseSize * sNoiseSize> noise;
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
        // Bindings 1,2 written in execute, they depend on the Input parameter
        mSSAO_Descriptor = mRHI->createDescriptor({
            .bindings = {
                { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                { 2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                { 3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
            },
            .setCount = RHI::gFramesInFlight,
            .debugName = "SSAO_Descriptor",
        });

        using enum vk::ImageUsageFlagBits;
        for (size_t i = 0; i < mSSAO_Result.size(); i++)
        {
            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mSSAO_Noise);
            mSSAO_Descriptor->write(i, descriptorWrite);

            mSSAO_Result[i] = makeRenderTarget(mRHI.get(), "SSAO_Result", vk::Format::eR32Sfloat);
        }

        const auto graphicsPS = RHI::GraphicsPS()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentState(1)
            .addAttachmentFormat(mSSAO_Result[0]->getProperties().format);
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("SSAO_Main")
            .addShader("FSQuad.vert.spv")
            .addShader("SSAO_new.frag.spv")
            .addDescriptorLayout(0, mSSAO_Descriptor.get())
            .setPushConstant<PushConstant>(vk::ShaderStageFlagBits::eFragment);

        mSSAO_Pipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }

    void SSAOPass::createResources_Blur() noexcept
    {
        mBlur_Descriptor = mRHI->createDescriptor({
            .bindings = {
                { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
            },
            .setCount = RHI::gFramesInFlight,
            .debugName = "SSAO_Blur_Descriptor",
        });

        using enum vk::ImageUsageFlagBits;
        for (size_t i = 0; i < mSSAO_Result.size(); i++)
        {
            mBlur_Result[i] = makeRenderTarget(mRHI.get(), "SSAO_Blur_Result", vk::Format::eR32Sfloat);

            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mSSAO_Result[i]);
            mBlur_Descriptor->write(i, descriptorWrite);
        }

        const auto graphicsPS = RHI::GraphicsPS()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentState(1)
            .addAttachmentFormat(mBlur_Result[0]->getProperties().format);
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("SSAO_Blur")
            .addShader("FSQuad.vert.spv")
            .addShader("SSAO_Blur.frag.spv")
            .addDescriptorLayout(0, mBlur_Descriptor.get())
            .setPushConstant<PushConstant>(vk::ShaderStageFlagBits::eFragment);

        mBlur_Pipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }

    void SSAOPass::execute_SSAO(const Input& input, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        const auto i = frameData.currentFrame;

        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeCombinedImageSampler(1, 0, vk::ImageLayout::eShaderReadOnlyOptimal, input.positions)
            .writeCombinedImageSampler(2, 0, vk::ImageLayout::eShaderReadOnlyOptimal, input.normals)
            .writeCombinedImageSampler(3, 0, vk::ImageLayout::eShaderReadOnlyOptimal, input.viewZ);
        mSSAO_Descriptor->write(i, descriptorWrite);

        RHI::Barrier()
            .addBarrier(mSSAO_Result[i]->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mSSAO_Noise->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(input.positions->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(input.normals->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(input.viewZ->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mSSAO_Kernel->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::Fragment))
            .insert(pCommandList);

        RHI::Rendering()
            .setLabel("SSAO_Main_Pass")
            .setRenderArea(mSSAO_Result[i]->getProperties().extent)
            .addAttachment(mSSAO_Result[i])
            .setViewportScissor(pCommandList)
            .execute(pCommandList, [&](RHI::CommandList* cmd)
            {
                const auto [w, h] = mSSAO_Result[i]->getProperties().extent;
                const auto pushConstant = PushConstant {
                    .cameraBuffer = input.cameraBuffer,
                    .kernelBuffer = mSSAO_Kernel->getAddress(),
                    .renderDim    = { w, h },
                    .noiseDim     = { sNoiseSize, sNoiseSize },
                };
                cmd->bindPipeline(mSSAO_Pipeline.get());
                cmd->bindDescriptorSet(mSSAO_Descriptor->getSet(i), 0);
                cmd->pushConstants(&pushConstant);
                cmd->draw(3, 1, 0, 0);
            });
    }

    void SSAOPass::execute_Blur(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        const auto i = frameData.currentFrame;

        RHI::Barrier()
            .addBarrier(mSSAO_Result[i]->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mBlur_Result[i]->getBarrier(RHI::ImageUsage::ColorAttachment))
            .insert(pCommandList);

        RHI::Rendering()
            .setLabel("SSAO_Blur_Pass")
            .setRenderArea(mBlur_Result[i]->getProperties().extent)
            .addAttachment(mBlur_Result[i])
            .setViewportScissor(pCommandList)
            .execute(pCommandList, [&](RHI::CommandList* cmd)
            {
                cmd->bindPipeline(mBlur_Pipeline.get());
                cmd->bindDescriptorSet(mBlur_Descriptor->getSet(i), 0);
                cmd->draw(3, 1, 0, 0);
            });
    }
}
