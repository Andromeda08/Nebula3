#include "TonemapPass.hpp"

#include "Templates.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace nbl
{
    TonemapPass::TonemapPass(const Tonemap_Params& params)
    : mRHI(params.rhi)
    , mInput(params.color)
    {
        createResources();
        createPipeline();
    }

    void TonemapPass::execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        pCommandList->beginLabel("Tonemap_Pass");

        RHI::Barrier()
            .addBarrier(mInput->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mOutput->getBarrier(RHI::ImageUsage::ColorAttachment))
            .insert(pCommandList);

        RHI::Rendering()
            .setLabel("Tonemap_RenderPass")
            .setRenderArea(mOutput->getProperties().extent)
            .addAttachment(mOutput)
            .setViewportScissor(pCommandList)
            .execute(pCommandList, [&](RHI::CommandList* cmd) -> void
            {
                cmd->bindPipeline(mPipeline.get());
                cmd->pushConstants(&mPushConstant);
                cmd->bindDescriptorSet(mDescriptor->getSet(), 0);
                cmd->draw(3, 1, 0, 0);
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
        mOutput = makeRenderTarget(mRHI.get(), "Tonemap_Result", mInput->getProperties().format);

        mDescriptor = mRHI->createDescriptor({
            .bindings = {
                { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
            },
            .setCount = 1,
            .debugName = "Tonemap_Descriptor",
        });

        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput);
        mDescriptor->write(0, descriptorWrite);
    }

    void TonemapPass::createPipeline() noexcept
    {
        const auto graphicsPS = RHI::GraphicsPS()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentState(1)
            .addAttachmentFormat(mOutput->getProperties().format);
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("Tonemap")
            .addShader("FSQuad.vert.spv")
            .addShader("Tonemap.frag.spv")
            .addDescriptorLayout(0, mDescriptor.get())
            .setPushConstant<PushConstant>(vk::ShaderStageFlagBits::eFragment);

        mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }
}
