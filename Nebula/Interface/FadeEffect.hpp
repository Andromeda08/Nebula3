#pragma once

#include <glm/glm.hpp>
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/Frame.hpp"
#include "VulkanRHI/VulkanRHI.hpp"
#include "VulkanRHI/Commands/CommandList.hpp"
#include "VulkanRHI/Render/Pipeline.hpp"
#include "Level/Render/Templates.hpp"
#include "Math/DeltaTime.hpp"

namespace nbl
{
    class FadeEffect
    {
        struct PushConstant
        {
            glm::vec4 srcColor = glm::vec4(0.0f);
            float     progress = 0.0f;
        };
    public:
        explicit FadeEffect(const SPtr<RHI::VulkanRHI>& rhi)
        : mRHI(rhi)
        {
            using enum vk::ImageUsageFlagBits;
            for (size_t i = 0; i < mOutput.size(); i++)
            {
                mOutput[i] = makeRenderTarget(mRHI.get(), fmt::format("FadeResult_{}", i));
            }

            mDescriptor = mRHI->createDescriptor({
                .bindings = {
                    { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                },
                .setCount = 2,
                .debugName = "FadeDescriptor",
            });

            const auto graphicsPS = RHI::GraphicsPS()
                .setCullMode(vk::CullModeFlagBits::eNone)
                .addDefaultAttachmentState(1)
                .addAttachmentFormat(mOutput[0]->getProperties().format);
            const auto pipelineInfo = RHI::PipelineCommon()
                .setLabel("FadeEffect")
                .addShader("FSQuad.vert.spv")
                .addShader("FadeEffect.frag.spv")
                .addDescriptorLayout(0, mDescriptor.get())
                .setPushConstant<PushConstant>(vk::ShaderStageFlagBits::eFragment);

            mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
        }

        void trigger()
        {
            if (mIsRunning)
            {
                return;
            }

            mIsRunning = true;
            mStartTime = std::chrono::high_resolution_clock::now();
        }

        void execute(const SPtr<RHI::Image>& pSource, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
        {
            if (!mIsRunning)
            {
                return;
            }

            pCommandList->beginLabel("FadeEffect");

            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, pSource);
            mDescriptor->write(frameData.currentFrame, descriptorWrite);

            const auto currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<float> delta = currentTime - mStartTime;
            const float dt = delta.count();

            const PushConstant pushConstant = {
                .srcColor = glm::vec4(0.0f),
                .progress = glm::min(1.0f, dt / mLength),
            };

            RHI::Barrier()
                .addBarrier(pSource->getBarrier(RHI::ImageUsage::ShaderReadOnly))
                .addBarrier(mOutput[frameData.currentFrame]->getBarrier(RHI::ImageUsage::ColorAttachment))
                .insert(pCommandList);

            RHI::Rendering()
                .setLabel("FadeEffect_RenderPass")
                .setRenderArea(mOutput[frameData.currentFrame]->getProperties().extent)
                .addAttachment(mOutput[frameData.currentFrame])
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

        [[nodiscard]] const SPtr<RHI::Image>& getResult(const uint32_t frame)
        {
            return mOutput[frame];
        }

    private:
        bool                                           mIsRunning = false;
        float                                          mLength    = 1.0f;
        std::chrono::high_resolution_clock::time_point mStartTime;

        SPtr<RHI::VulkanRHI>            mRHI;
        PerFrameArray<SPtr<RHI::Image>> mOutput;
        SPtr<RHI::Descriptor>           mDescriptor;
        SPtr<RHI::GraphicsPipeline2>    mPipeline;
    };
}
