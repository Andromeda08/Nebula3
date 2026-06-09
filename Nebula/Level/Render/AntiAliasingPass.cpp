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

    void AntiAliasingPass::execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        pCommandList->beginLabel("AntiAliasing_Pass");

        RHI::Barrier()
            .addBarrier(mInput->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mOutput->getBarrier(RHI::ImageUsage::ColorAttachment))
            .insert(pCommandList);

        const auto pushConstant = PushConstant {
            1.0f / static_cast<float>(mInput->getProperties().extent.width),
            1.0f / static_cast<float>(mInput->getProperties().extent.height),
        };

        RHI::Rendering()
            .setLabel("AntiAliasing_RenderPass")
            .setRenderArea(mOutput->getProperties().extent)
            .addAttachment(mOutput)
            .setViewportScissor(pCommandList)
            .execute(pCommandList, [&](RHI::CommandList* cmd) -> void
            {
                cmd->bindPipeline(mPipeline.get());
                cmd->pushConstants(&pushConstant);
                cmd->bindDescriptorSet(mDescriptor->getSet(), 0);
                cmd->draw(3, 1, 0, 0);
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
        const auto graphicsPS = RHI::GraphicsPS()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentState(1)
            .addAttachmentFormat(mOutput->getProperties().format);
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("FXAA")
            .addShader("FXAA.vert.spv")
            .addShader("FXAA.frag.spv")
            .addDescriptorLayout(0, mDescriptor.get())
            .setPushConstant<PushConstant>(vk::ShaderStageFlagBits::eFragment);

        mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }
}
