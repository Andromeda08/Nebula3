#pragma once

#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/Rendering.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

#include <glm/glm.hpp>

namespace viz
{
    struct PushConstants {
        glm::vec4 bboxMin;
        glm::vec4 bboxMax;
        int numAtoms;
        float radius;
        float scale;
    };

    class ComputePrePass
    {
    public:
        ComputePrePass(SPtr<RHI::VulkanRHI> rhi, const SPtr<RHI::Buffer>& positions, const std::vector<glm::vec3>& atomPositions)
        : mRHI(std::move(rhi))
        , mPositions(positions)
        , mTextureExtents(100, 100, 100)
        , mPC(glm::vec4(INT_MAX), glm::vec4(INT_MIN), atomPositions.size(), .1f, 1.f)
        {
            mImage3D = mRHI->createImage3D({
                .extent     = mTextureExtents,
                .format     = vk::Format::eR32G32B32A32Sfloat,
                .usageFlags = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
                .debugName = "MoleculeSDF"
            });

            mRHI->getGraphicsQueue()->immediate([&](const auto* commandList) -> void {
               const auto barrier = RHI::Barrier()
                    .addBarrier(mImage3D->getBarrier(RHI::ImageUsage::General));
                barrier.insert(commandList);
            });

            // Calculate molecule bounding box
            for (const auto& p : atomPositions) {
                if (p.x < mPC.bboxMin.x) mPC.bboxMin.x = p.x;
                if (p.y < mPC.bboxMin.y) mPC.bboxMin.y = p.y;
                if (p.z < mPC.bboxMin.z) mPC.bboxMin.z = p.z;

                if (p.x > mPC.bboxMax.x) mPC.bboxMax.x = p.x;
                if (p.y > mPC.bboxMax.y) mPC.bboxMax.y = p.y;
                if (p.z > mPC.bboxMax.z) mPC.bboxMax.z = p.z;
            }

            vk::PushConstantRange pcr;
            pcr.offset = 0;
            pcr.size = sizeof(PushConstants);
            pcr.stageFlags = vk::ShaderStageFlagBits::eCompute;

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
                .writeStorageImages(0, 1, &imageInfo)
                .writeStorageBuffers(1, 1, &bufferInfo);
            mDescriptor->write_old(write);

            mPipeline = mRHI->createComputePipeline(RHI::ComputePipelineCreateInfo()
                .setComputeShader({ Configuration::getShaderFilePath("viz.comp.spv"), vk::ShaderStageFlagBits::eCompute, "main" })
                .setPushConstantRange(pcr)
                .addDescriptorSetLayout(mDescriptor->getLayout())
                .setDebugName("VizCompute"));
        }

        void execute(const RHI::CommandList* commandList) const
        {
            mPipeline->bind(commandList->getHandle());
            mPipeline->pushConstants(commandList->getHandle(), &mPC);
            mPipeline->bindDescriptorSet(commandList->getHandle(), mDescriptor->getSet(0));
            mPipeline->dispatch(commandList->getHandle(), ceil(mTextureExtents.width/4.), ceil(mTextureExtents.height/4.), ceil(mTextureExtents.depth/4.));
        }

        [[nodiscard]] SPtr<RHI::Image3D> getSDFTexture3D() const noexcept
        {
            return mImage3D;
        }

        ~ComputePrePass() = default;

    private:
        SPtr<RHI::VulkanRHI>        mRHI;
        SPtr<RHI::Descriptor>       mDescriptor;
        SPtr<RHI::ComputePipeline>  mPipeline;

        SPtr<RHI::Image3D>          mImage3D;
        SPtr<RHI::Buffer>           mPositions;
        SPtr<RHI::Buffer>           mConfig;

        PushConstants               mPC;
        vk::Extent3D                mTextureExtents;
    };
}
