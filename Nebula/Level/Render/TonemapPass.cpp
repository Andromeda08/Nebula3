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

    void TonemapPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        pCommandList->beginLabel("Tonemap_Pass");

        RHI::Barrier()
            .addBarrier(mInput->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mOutput->getBarrier(RHI::ImageUsage::ColorAttachment))
            .insert(pCommandList);

        mRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
            mPipeline->bind(cmd);
            mPipeline->pushConstants(cmd, &mPushConstant);
            mPipeline->bindDescriptorSet(cmd, mDescriptor->getSet());
            cmd->getHandle().draw(3, 1, 0, 0);
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
        mRenderPass = mRHI->createRenderPass({
            .renderArea = getRenderAreaForAttachment(mOutput.get()),
            .colorAttachments = { makeAttachment(mOutput) },
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
}
