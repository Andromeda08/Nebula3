#pragma once

#include "Shared.hpp"
#include "Level/Render/Templates.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class HybridHair_DebugCompose
    {
    public:
        explicit HybridHair_DebugCompose(const SPtr<RHI::VulkanRHI>& rhi, HairShared* pHairShared)
        : mRHI(rhi)
        , mShared(pHairShared)
        {
            createResources();
            createPipeline();
        }

        void execute(
            RHI::CommandList*       pCommandList,
            const RHI::FrameData&   frameData,
            const SPtr<RHI::Image>& meshOutput,
            const SPtr<RHI::Image>& computeOutput
        ) const noexcept
        {
            pCommandList->beginLabel("Debug Compose");

            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, meshOutput)
                .writeCombinedImageSampler(1, 0, vk::ImageLayout::eShaderReadOnlyOptimal, computeOutput);
            mDescriptor->write(frameData.currentFrame, descriptorWrite);

            RHI::Barrier()
                .addBarrier(mOutput[frameData.currentFrame]->getBarrier(RHI::ImageUsage::ColorAttachment))
                .addBarrier(meshOutput->getBarrier(RHI::ImageUsage::ShaderReadOnly))
                .addBarrier(computeOutput->getBarrier(RHI::ImageUsage::ShaderReadOnly))
                .insert(pCommandList);

            pCommandList->setViewportScissor(mViewport, mScissor);

            RHI::Rendering()
                .setRenderArea(mScissor)
                .addAttachment(mOutput[frameData.currentFrame])
                .setLabel(fmt::format("Hair_Classic_RenderPass"))
                .execute(pCommandList, [&](const RHI::CommandList* cmd) -> void
                {
                    mPipeline->bind(cmd);
                    mPipeline->pushConstants(cmd, &mShared->config.debugAlphaBlend);
                    mPipeline->bindDescriptorSet(cmd, mDescriptor->getSet(frameData.currentFrame));
                    cmd->getHandle().draw(3, 1, 0, 0);
                });

            pCommandList->endLabel();
        }

        [[nodiscard]] const SPtr<RHI::Image>& getResult(const uint32_t currentFrame) const noexcept
        {
            return mOutput[currentFrame];
        }

    private:
        void createResources() noexcept
        {
            for (size_t i = 0; i < mOutput.size(); i++)
            {
                mOutput[i] = makeRenderTarget(mRHI.get(), fmt::format("HybridHair_Composite_Target_{}", i));
            }

            mDescriptor = mRHI->createDescriptor({
                .bindings  = {
                    // Mesh Output
                { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                    // Compute Output
                { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                },
                .setCount  = RHI::gFramesInFlight,
                .debugName = "HybridHair_Composite_Descriptor",
            });
        }

        void createPipeline() noexcept
        {
            mScissor = getRenderAreaForAttachment(mOutput[0].get());
            mViewport = vk::Viewport {
                0.0f, 0.0f,
                static_cast<float>(mScissor.extent.width), static_cast<float>(mScissor.extent.height),
                0.0f, 1.0f
            };

            const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
                .setPushConstantRange<float>(vk::ShaderStageFlagBits::eFragment)
                .addDescriptorSetLayout(mDescriptor->getLayout())
                .setStateInfo(RHI::makeGraphicsStateInfo([&](RHI::GraphicsPipelineStateInfo& stateInfo)
                {
                    stateInfo
                        .addDefaultAttachmentStates(1)
                        .setCullMode(vk::CullModeFlagBits::eNone);
                }))
                .addShader({ Configuration::getShaderFilePath("FSQuad.vert.spv"), vk::ShaderStageFlagBits::eVertex })
                .addShader({ Configuration::getShaderFilePath("HybridHair_DebugCompose.frag.spv"), vk::ShaderStageFlagBits::eFragment })
                .addColorAttachmentFormat(mOutput[0]->getProperties().format)
                .setDebugName("HybridHair_DebugCompose_Pipeline");

            mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
        }

        SPtr<RHI::VulkanRHI>                    mRHI;
        HairShared*                             mShared;

        PerFrameArray<SPtr<RHI::Image>>         mOutput;

        vk::Rect2D                              mScissor;
        vk::Viewport                            mViewport;

        SPtr<RHI::Descriptor>                   mDescriptor;
        SPtr<RHI::Pipeline>                     mPipeline;
    };
}