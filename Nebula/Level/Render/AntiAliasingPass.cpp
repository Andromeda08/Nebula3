#include "AntiAliasingPass.hpp"

#include "Templates.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace nbl
{
    AntiAliasingPass::AntiAliasingPass(const AntiAliasing_Params& params)
    : mRHI(params.rhi)
    , mInput(params.input)
    {
        createResources();
        createPipeline();
    }

    void AntiAliasingPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        pCommandList->beginLabel("Tonemap_Pass");

        RHI::Barrier()
            .addBarrier(mInput->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mOutput->getBarrier(RHI::ImageUsage::ColorAttachment))
            .insert(pCommandList);

        const auto pushConstant = PushConstant {
            1.0f / static_cast<float>(mInput->getProperties().extent.width),
            1.0f / static_cast<float>(mInput->getProperties().extent.height),
        };

        mRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
            mPipeline->bind(cmd);
            mPipeline->pushConstants(cmd, &pushConstant);
            mPipeline->bindDescriptorSet(cmd, mDescriptor->getSet());
            cmd->getHandle().draw(3, 1, 0, 0);
        });

        pCommandList->endLabel();
    }

    SPtr<RHI::Image> AntiAliasingPass::getResult() const noexcept
    {
        return mOutput;
    }

    void AntiAliasingPass::createResources() noexcept
    {
        using enum vk::ImageUsageFlagBits;
        mOutput = makeRenderTarget(mRHI.get(), "AntiAliasing_Result", mInput->getProperties().format);

        mDescriptor = mRHI->createDescriptor({
            .bindings = {
                { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
            },
            .setCount = 1,
            .debugName = "AntiAliasing_Descriptor",
        });

        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput);
        mDescriptor->write(0, descriptorWrite);
    }

    void AntiAliasingPass::createPipeline() noexcept
    {
        mRenderPass = mRHI->createRenderPass({
            .renderArea = getRenderAreaForAttachment(mOutput.get()),
            .colorAttachments = { makeAttachment(mOutput) },
            .label = "AntiAliasing_RenderPass",
        });

        const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .setPushConstantRange<PushConstant>(vk::ShaderStageFlagBits::eFragment)
            .addDescriptorSetLayout(mDescriptor->getLayout())
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .setCullMode(vk::CullModeFlagBits::eNone)
                .addDefaultAttachmentStates(1))
            .addShader({ Configuration::getShaderFilePath("FXAA.vert.spv").string(), vk::ShaderStageFlagBits::eVertex })
            .addShader({ Configuration::getShaderFilePath("FXAA.frag.spv").string(), vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mOutput->getProperties().format)
            .setDebugName("AntiAliasing_Pipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }
}
