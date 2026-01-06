#pragma once

#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/Rendering.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace viz
{
    class ComputePrePass final : public IPass
    {
    public:
        explicit ComputePrePass(const SPtr<RHI::VulkanRHI>& rhi, const SPtr<RHI::Buffer>& positions)
        : mRHI(rhi)
        , mPositions(positions)
        {
            mImage3D = mRHI->createImage3D({
                .extent     = { 100, 100, 100 },
                .format     = vk::Format::eR32G32B32A32Sfloat,
                .usageFlags = vk::ImageUsageFlagBits::eStorage,
                .debugName = "MoleculeSDF"
            });
            mRHI->getGraphicsQueue()->immediate([&](const auto* commandList) -> void {
               const auto barrier = RHI::Barrier()
                    .addBarrier(mImage3D->getBarrier(RHI::ImageUsage::General));
                barrier.insert(commandList);
            });

            mDescriptor = mRHI->createDescriptor({
                .bindings     = {
                    vk::DescriptorSetLayoutBinding().setBinding(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageImage).setStageFlags(vk::ShaderStageFlagBits::eCompute),
                    vk::DescriptorSetLayoutBinding().setBinding(1).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer).setStageFlags(vk::ShaderStageFlagBits::eCompute),
                },
                .setCount     = 1,
                .debugName    = "ComputePrePassDescriptor",
            });

            const auto imageInfo = vk::DescriptorImageInfo { nullptr, mImage3D->getImageView(), vk::ImageLayout::eGeneral };
            const auto bufferInfo = vk::DescriptorBufferInfo { mPositions->getHandle(), 0, mPositions->getSize() };
            const auto write = RHI::DescriptorWriteInfo()
                .setSetIndex(0)
                .writeCombinedImageSamplers(0, 1, &imageInfo)
                .writeStorageBuffers(1, 1, &bufferInfo);
            mDescriptor->write_old(write);

            // mPipeline = mRHI->createComputePipeline(RHI::ComputePipelineCreateInfo()
            //     .setComputeShader({ Configuration::getShaderFilePath("viz.comp.spv"), vk::ShaderStageFlagBits::eCompute, "main" })
            //     .addDescriptorSetLayout(mDescriptor->getLayout())
            //     .setDebugName("VizCompute"));
        }

        void execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) override
        {
            // TODO: exec. pipeline
        }

        ~ComputePrePass() override = default;

    private:
        SPtr<RHI::VulkanRHI>        mRHI;
        SPtr<RHI::Descriptor>       mDescriptor;
        SPtr<RHI::ComputePipeline>  mPipeline;

        SPtr<RHI::Image3D>          mImage3D;
        SPtr<RHI::Buffer>           mPositions;
    };
}
