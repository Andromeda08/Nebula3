#include "SDFComputePass.hpp"

#include "VulkanRHI/Barrier.hpp"

namespace Molecule
{
    namespace detail
    {
        void computeMoleculeBoundingBox(glm::vec4& min, glm::vec4& max, const std::vector<glm::vec3>& positions) noexcept
        {
            constexpr float probeR = 3.0f;
            for (const auto& pos : positions)
            {
                if (pos.x < min.x) { min.x = pos.x; }
                if (pos.y < min.y) { min.y = pos.y; }
                if (pos.z < min.z) { min.z = pos.z; }

                if (pos.x > max.x) { max.x = pos.x; }
                if (pos.y > max.y) { max.y = pos.y; }
                if (pos.z > max.z) { max.z = pos.z; }
            }

            min -= probeR;
            max += probeR;
        }
    }

    SDFComputePass::SDFComputePass(SPtr<RHI::VulkanRHI> rhi, const std::vector<glm::vec3>& atomPositions)
    : mRHI(std::move(rhi))
    , mPushConstants(glm::vec4(INT_MAX), glm::vec4(INT_MIN), atomPositions.size(), .4f, .25f)
    , mTextureExtent(128, 128, 128)
    {
        // Create 3D SDF Texture
        mImage3D = mRHI->createImage3D({
            .extent     = mTextureExtent,
            .format     = vk::Format::eR32G32B32A32Sfloat,
            .usageFlags = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
            .debugName  = "MoleculeSDF"
        });

        // Calculate molecule bounding box
        detail::computeMoleculeBoundingBox(mPushConstants.bboxMin, mPushConstants.bboxMax, atomPositions);

        // Prepare position data upload
        #pragma region

        std::vector<glm::vec4> alignedPos;
        alignedPos.reserve(atomPositions.size());
        for (const auto& p : atomPositions)
        {
            alignedPos.emplace_back(p, 0.f);
        }

        const auto positionsSize = alignedPos.size() * sizeof(glm::vec4);
        const auto staging = mRHI->createBuffer({ positionsSize, RHI::BufferType::Staging });
        staging->setData(alignedPos.data(), positionsSize);

        mPositions = mRHI->createBuffer({
            .size  = positionsSize,
            .type  = RHI::BufferType::Storage,
            .label = "Molecule Positions",
        });

        #pragma endregion

        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* commandList) -> void {
            // Initial layout transition for mImage3D
            const auto barrier = RHI::Barrier()
                .addBarrier(mImage3D->getBarrier(RHI::ImageUsage::General));
            barrier.insert(commandList);

            // Upload positions from staging to device local buffer
            const auto copy = vk::BufferCopy2().setSrcOffset(0).setDstOffset(0).setSize(positionsSize);
            const auto info = vk::CopyBufferInfo2()
                .setSrcBuffer(staging->getHandle())
                .setDstBuffer(mPositions->getHandle())
                .setRegions(copy);
            commandList->getHandle().copyBuffer2(info);
        });

        // Descriptor
        mDescriptor = mRHI->createDescriptor({
            .bindings = {
                vk::DescriptorSetLayoutBinding { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
                vk::DescriptorSetLayoutBinding { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
            },
            .setCount = 1,
            .debugName = "SDFComputePassDescriptor",
        });

        // TODO: Update when there are 3D Images in RHI::DescriptorWrite
        const auto imageInfo = vk::DescriptorImageInfo { nullptr, mImage3D->getImageView(), vk::ImageLayout::eGeneral };
        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeStorageImageInfos(0, std::span{&imageInfo, 1})
            .writeStorageBuffer(1, mPositions);
        mDescriptor->writeAll(descriptorWrite);

        // Pipeline
        mDispatchSize = {
            static_cast<uint32_t>(glm::ceil(static_cast<double>(mTextureExtent.width)  / 4.0)),
            static_cast<uint32_t>(glm::ceil(static_cast<double>(mTextureExtent.height) / 4.0)),
            static_cast<uint32_t>(glm::ceil(static_cast<double>(mTextureExtent.depth)  / 4.0)),
        };
        auto pipelineInfo = RHI::ComputePipelineCreateInfo()
            .setComputeShader({ Configuration::getShaderFilePath("viz.comp.spv"), vk::ShaderStageFlagBits::eCompute, "main" })
            .setPushConstantRange({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushConstants) })
            .addDescriptorSetLayout(mDescriptor->getLayout())
            .setDebugName("SDFComputePass");
        mPipeline = mRHI->createComputePipeline(pipelineInfo);
    }

    void SDFComputePass::execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) const
    {
        commandList->beginLabel("SDFComputePass");

        const auto barrier = RHI::Barrier()
                .addBarrier(mImage3D->getBarrier(RHI::ImageUsage::StorageImage));
        barrier.insert(commandList);

        mPipeline->bind(commandList->getHandle());
        mPipeline->pushConstants(commandList->getHandle(), &mPushConstants);
        mPipeline->bindDescriptorSet(commandList->getHandle(), mDescriptor->getSet(0));
        mPipeline->dispatch(commandList->getHandle(), mDispatchSize[0], mDispatchSize[1], mDispatchSize[2]);

        commandList->endLabel();
    }

    SPtr<RHI::Image3D> SDFComputePass::getSDFTexture3D() const noexcept
    {
        return mImage3D;
    }
}
