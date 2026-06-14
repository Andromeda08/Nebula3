#pragma once

#include <glm/glm.hpp>
#include "VulkanRHI/VulkanRHI.hpp"
#include "VulkanRHI/Render/Pipeline.hpp"

namespace nbl
{
    /**
     * A Bloom render pass implementing the Dual Kawase Blur
     */
    class BloomPass
    {
        struct Params
        {
            uint32_t        downsamples = 4;
            vk::Extent2D    extent;
        };
        struct PushConstants
        {
            glm::vec2 resRcp;
            uint32_t  maxDownsamples;
            float     offset   = 1.0f;
            float     strength = 1.0f;
        };
    public:
        nbl_DisableCopy(BloomPass);

        BloomPass(const Params& params, const SPtr<RHI::VulkanRHI>& rhi)
        : mRHI(rhi)
        {
            for (uint32_t i = 0; i < RHI::gFramesInFlight; i++)
            {
                using enum vk::ImageUsageFlagBits;
                mTargets[i] = mRHI->createImage({
                    .extent         = params.extent,
                    .format         = vk::Format::eR16G16B16A16Sfloat,
                    .usageFlags     = eColorAttachment | eStorage | eTransferSrc | eTransferDst | eSampled,
                    .samples        = vk::SampleCountFlagBits::e1,
                    .createSampler  = true,
                    .mipmapping     = true,
                    .debugName      = fmt::format("BloomTarget_{}", i),
                });
            }

            mDescriptor = mRHI->createDescriptor({
                .bindings  = {
                    { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                    { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                },
                .setCount  = RHI::gFramesInFlight,
                .debugName = "Bloom",
            });

            /* Downsample Pipeline */
            {
                const auto graphicsPS = RHI::GraphicsPS()
                    .setCullMode(vk::CullModeFlagBits::eNone)
                    .addDefaultAttachmentState(1)
                    .addAttachmentFormat(mTargets[0]->getProperties().format);
                const auto pipelineInfo = RHI::PipelineCommon()
                    .setPushConstant<PushConstants>(vk::ShaderStageFlagBits::eFragment)
                    .addDescriptorLayout(0, mDescriptor.get())
                    .setLabel("DualKawase_DownSample")
                    .addShader("FSQuad.vert.spv")
                    .addShader("DualKawase_Down.frag.spv");

                mDownSample = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
            }

            /* Upsample Pipeline */
            {
                const auto graphicsPS = RHI::GraphicsPS()
                    .setCullMode(vk::CullModeFlagBits::eNone)
                    .addDefaultAttachmentState(1)
                    .addAttachmentFormat(mTargets[0]->getProperties().format);
                const auto pipelineInfo = RHI::PipelineCommon()
                    .setPushConstant<PushConstants>(vk::ShaderStageFlagBits::eFragment)
                    .addDescriptorLayout(0, mDescriptor.get())
                    .setLabel("DualKawase_UpSample")
                    .addShader("FSQuad.vert.spv")
                    .addShader("DualKawase_Up.frag.spv");

                mUpSample = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
            }
        }

        ~BloomPass() = default;

        void execute(const SPtr<RHI::Image>& input, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
        {
            const auto& target = mTargets[frameData.currentFrame];

            // Downsample
            
        }

        [[nodiscard]] const SPtr<RHI::Image>& getResult(uint32_t currentFrame) const noexcept;

    private:
        SPtr<RHI::VulkanRHI>            mRHI;

        PerFrameArray<SPtr<RHI::Image>> mTargets;

        SPtr<RHI::Descriptor>           mDescriptor;
        UPtr<RHI::GraphicsPipeline2>    mDownSample;
        UPtr<RHI::GraphicsPipeline2>    mUpSample;
    };

}
