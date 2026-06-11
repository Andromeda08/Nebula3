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

        const auto& input  = mInput[frameData.currentFrame];
        const auto& output = mOutput[frameData.currentFrame];

        RHI::Barrier()
            .addBarrier(input->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(output->getBarrier(RHI::ImageUsage::ColorAttachment))
            .insert(pCommandList);

        const auto pushConstant = PushConstant {
            1.0f / static_cast<float>(input->getProperties().extent.width),
            1.0f / static_cast<float>(input->getProperties().extent.height),
        };

        RHI::Rendering()
            .setLabel("AntiAliasing_RenderPass")
            .setRenderArea(output->getProperties().extent)
            .addAttachment(output)
            .setViewportScissor(pCommandList)
            .execute(pCommandList, [&](RHI::CommandList* cmd) -> void
            {
                cmd->bindPipeline(mPipeline.get());
                cmd->pushConstants(&pushConstant);
                cmd->bindDescriptorSet(mDescriptor->getSet(frameData.currentFrame), 0);
                cmd->draw(3, 1, 0, 0);
            });

        pCommandList->endLabel();
    }

    const SPtr<RHI::Image>& AntiAliasingPass::getResult(const uint32_t currentFrame) const noexcept
    {
        return mOutput[currentFrame];
    }

    const PerFrameArray<SPtr<RHI::Image>>& AntiAliasingPass::getResults() const noexcept
    {
        return mOutput;
    }

    void AntiAliasingPass::createResources() noexcept
    {
        using enum vk::ImageUsageFlagBits;

        mDescriptor = mRHI->createDescriptor({
            .bindings = {{ 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }},
            .setCount = 2,
            .debugName = "AntiAliasing_Descriptor",
        });

        for (size_t i = 0; i < mInput.size(); i++)
        {
            mOutput[i] = makeRenderTarget(mRHI.get(), "AntiAliasing_Result", mInput[i]->getProperties().format);

            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput[i]);
            mDescriptor->write(i, descriptorWrite);
        }
    }

    void AntiAliasingPass::createPipeline() noexcept
    {
        const auto graphicsPS = RHI::GraphicsPS()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentState(1)
            .addAttachmentFormat(mOutput[0]->getProperties().format);
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("FXAA")
            .addShader("FXAA.vert.spv")
            .addShader("FXAA.frag.spv")
            .addDescriptorLayout(0, mDescriptor.get())
            .setPushConstant<PushConstant>(vk::ShaderStageFlagBits::eFragment);

        mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }
}
