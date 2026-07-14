#include "TonemapPass.hpp"

#include "Templates.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace nbl
{
    TonemapPass::TonemapPass(const Tonemap_Params& params)
    : mRHI(params.rhi)
    {
        using enum vk::ImageUsageFlagBits;
        for (size_t i = 0; i < mOutput.size(); i++)
        {
            mOutput[i] = makeRenderTarget(mRHI.get(), "Tonemap_Result", params.outputFormat);
        }

        mDescriptor = mRHI->createDescriptor({
            .bindings  = {{ 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }},
            .setCount  = RHI::gFramesInFlight,
            .debugName = "Tonemap_Descriptor",
        });

        const auto graphicsPS = RHI::GraphicsPS()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentState(1)
            .addAttachmentFormat(mOutput[0]->getProperties().format);
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("Tonemap")
            .addShader("FSQuad.vert.spv")
            .addShader("Tonemap.frag.spv")
            .addDescriptorLayout(0, mDescriptor.get())
            .setPushConstant<PushConstant>(vk::ShaderStageFlagBits::eFragment);

        mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }

    void TonemapPass::execute(const SPtr<RHI::Image>& input, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        pCommandList->beginLabel("Tonemap_Pass");

        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, input);
        mDescriptor->write(frameData.currentFrame, descriptorWrite);

        RHI::Barrier()
            .addBarrier(input->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mOutput[frameData.currentFrame]->getBarrier(RHI::ImageUsage::ColorAttachment))
            .insert(pCommandList);

        RHI::Rendering()
            .setLabel("Tonemap_RenderPass")
            .setRenderArea(mOutput[frameData.currentFrame]->getProperties().extent)
            .addAttachment(mOutput[frameData.currentFrame])
            .setViewportScissor(pCommandList)
            .execute(pCommandList, [&](RHI::CommandList* cmd) -> void
            {
                cmd->bindPipeline(mPipeline.get());
                cmd->pushConstants(&mPushConstant);
                cmd->bindDescriptorSet(mDescriptor->getSet(frameData.currentFrame), 0);
                cmd->draw(3, 1, 0, 0);
            });


        pCommandList->endLabel();
    }

    const SPtr<RHI::Image>& TonemapPass::getResult(const uint32_t currentFrame) const noexcept
    {
        return mOutput[currentFrame];
    }

    const PerFrameArray<SPtr<RHI::Image>>& TonemapPass::getResults() const noexcept
    {
        return mOutput;
    }
}
