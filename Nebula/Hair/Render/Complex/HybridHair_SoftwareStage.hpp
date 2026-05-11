#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "Shared.hpp"
#include "Core/Types.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class HybridHair_SoftwareStage
    {
        struct IndirectPushConstants
        {
            uint64_t smallTriangleCountBuffer;
            uint64_t indirectArgsBuffer;
            uint32_t groupSize;
            uint32_t maxGroups;
        };

        struct PushConstants
        {
            // Buffer References
            uint64_t trianglesBuffer;
            uint64_t smallTriangleCountBuffer;
            // Resolution
            float width;
            float height;
        };

        struct ResolvePushConstants
        {
            uint64_t colorsBuffer;
            uint32_t colorCount;
            float    width;
            float    height;
        };

    public:
        explicit HybridHair_SoftwareStage(const SPtr<RHI::VulkanRHI>& rhi, HairShared* pHairShared)
        : mRHI(rhi)
        , mShared(pHairShared)
        {
            mRenderResolution = mShared->config.renderResolution;

            createResources();
            createPipeline();
        }

        void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const
        {
            pCommandList->beginLabel("Software_Rasterizer");

            const auto& visibilityTarget = mShared->visibilityBuffer[frameData.currentFrame];
            const auto& colorTarget = mResolveRenderTargets[frameData.currentFrame];

            const auto& trianglesBuffer = mShared->trianglesBuffer[frameData.currentFrame];
            const auto& counterBuffer = mShared->smallTriangleCounterBuffer[frameData.currentFrame];

            const auto& indirectArgs = mIndirectArgsBuffer[frameData.currentFrame];

            // Clear render targets for both pipelines
            // ================================================
            pCommandList->beginLabel("Clear_Targets");
            {
                vk::ClearColorValue visibilityClear = {};
                visibilityClear.uint32[0] = sInvalidPrimitiveId;
                visibilityClear.uint32[1] = std::bit_cast<uint32_t>(1.0f);

                constexpr auto colorClear = vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f });

                RHI::Barrier()
                    .addBarrier(visibilityTarget->getBarrier(RHI::ImageUsage::Clear))
                    .addBarrier(colorTarget->getBarrier(RHI::ImageUsage::Clear))
                    .insert(pCommandList);

                pCommandList->getHandle().clearColorImage(
                    visibilityTarget->getImage(),
                    visibilityTarget->getState().layout,
                    visibilityClear,
                    visibilityTarget->getProperties().getSubresourceRange()
                );

                pCommandList->getHandle().clearColorImage(
                    colorTarget->getImage(),
                    colorTarget->getState().layout,
                    colorClear,
                    colorTarget->getProperties().getSubresourceRange()
                );
            }
            pCommandList->endLabel();

            // Compute indirect dispatch args
            // ================================================
            pCommandList->beginLabel("ComputeIndirectArgs");
            {
                constexpr uint32_t groupSize         = 64;
                const     uint32_t maxSmallTriangles = mShared->currentBufferSize[frameData.currentFrame] / sizeof(ScreenSpaceTriangle);
                const     uint32_t maxGroups         = (maxSmallTriangles + groupSize - 1) / groupSize;

                const auto pushConstants = IndirectPushConstants {
                    .smallTriangleCountBuffer = counterBuffer->getAddress(),
                    .indirectArgsBuffer       = indirectArgs->getAddress(),
                    .groupSize                = groupSize,
                    .maxGroups                = maxGroups,
                };

                RHI::Barrier()
                    .addBarrier(counterBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::Compute_Write))
                    .addBarrier(indirectArgs->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::Compute_Write))
                    .insert(pCommandList);

                mIndirectArgsPipeline->bind(pCommandList);
                mIndirectArgsPipeline->pushConstants(pCommandList, &pushConstants);
                mIndirectArgsPipeline->dispatch(pCommandList, 1, 1, 1);
            }
            pCommandList->endLabel();

            // Run visibility pipeline
            // ================================================
            pCommandList->beginLabel("Visibility");
            {
                const auto pushConstants = PushConstants {
                    .trianglesBuffer          = trianglesBuffer->getAddress(),
                    .smallTriangleCountBuffer = counterBuffer->getAddress(),
                    .width                    = static_cast<float>(mRenderResolution.width),
                    .height                   = static_cast<float>(mRenderResolution.height),
                };

                RHI::Barrier()
                    .addBarrier(visibilityTarget->getBarrier(RHI::ImageUsage::StorageImage))
                    .addBarrier(trianglesBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::StorageRead))
                    .addBarrier(counterBuffer->getBarrier(RHI::BufferUsage::Compute_Write, RHI::BufferUsage::StorageRead))
                    .addBarrier(indirectArgs->getBarrier(RHI::BufferUsage::Compute_Write, RHI::BufferUsage::DrawIndirect))
                    .insert(pCommandList);

                mPipeline->bind(pCommandList);
                mPipeline->pushConstants(pCommandList, &pushConstants);
                mPipeline->bindDescriptorSet(pCommandList, mDescriptor->getSet(frameData.currentFrame));

                pCommandList->getHandle().dispatchIndirect(indirectArgs->getHandle(), 0);
            }
            pCommandList->endLabel();

            // Run resolve pipeline
            // ================================================
            pCommandList->beginLabel("Resolve");
            {
                const auto pushConstants = ResolvePushConstants {
                    .colorsBuffer = mShared->colorsBuffer->getAddress(),
                    .colorCount   = mShared->sColorCount,
                    .width        = static_cast<float>(mRenderResolution.width),
                    .height       = static_cast<float>(mRenderResolution.height),
                };

                RHI::Barrier()
                   .addBarrier(visibilityTarget->getBarrier(RHI::ImageUsage::StorageImage))
                   .addBarrier(colorTarget->getBarrier(RHI::ImageUsage::StorageImage))
                   .insert(pCommandList);

                mResolvePipeline->bind(pCommandList);
                mResolvePipeline->pushConstants(pCommandList, &pushConstants);
                mResolvePipeline->bindDescriptorSet(pCommandList, mDescriptor->getSet(frameData.currentFrame));

                constexpr uint32_t groupSize = 8;
                const     uint32_t groupX    = (mRenderResolution.width  + groupSize - 1) / groupSize;
                const     uint32_t groupY    = (mRenderResolution.height + groupSize - 1) / groupSize;
                mResolvePipeline->dispatch(pCommandList, groupX, groupY, 1);
            }
            pCommandList->endLabel();

            pCommandList->endLabel();
        }

        [[nodiscard]] const SPtr<RHI::Image>& getResult(const uint32_t frameIndex) const
        {
            return mResolveRenderTargets[frameIndex];
        }

    private:
        SPtr<RHI::VulkanRHI>    mRHI;
        HairShared*             mShared;
        vk::Extent2D            mRenderResolution;

        static constexpr uint32_t sInvalidPrimitiveId = std::numeric_limits<uint32_t>::max();

        // Indirect Args Pipeline
        // ================================================
        PerFrameArray<SPtr<RHI::Buffer>> mIndirectArgsBuffer;
        UPtr<RHI::ComputePipeline>       mIndirectArgsPipeline;

        // Visibility Buffer Pipeline
        // ================================================
        SPtr<RHI::Descriptor>           mDescriptor;
        UPtr<RHI::ComputePipeline>      mPipeline;

        // Resolve Pipeline
        // ================================================
        PerFrameArray<SPtr<RHI::Image>> mResolveRenderTargets;
        SPtr<RHI::Descriptor>           mResolveDescriptor;
        UPtr<RHI::ComputePipeline>      mResolvePipeline;

        void createResources()
        {
            mDescriptor = mRHI->createDescriptor({
                .bindings  = {
                    // Visibility Buffer
                { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
                    // Color Target
                { 1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
                },
                .setCount  = RHI::gFramesInFlight,
                .debugName = "HybridHair_Descriptor",
            });

            for (size_t i = 0; i < RHI::gFramesInFlight; i++)
            {
                mIndirectArgsBuffer[i] = mRHI->createBuffer({
                    .size  = sizeof(uint32_t) * 3,
                    .type  = RHI::BufferType::Indirect,
                    .label = fmt::format("HybridHair_IndirectArgs_{}", i),
                });

                mResolveRenderTargets[i] = mRHI->createImage({
                    .extent     = mRenderResolution,
                    .format     = vk::Format::eR16G16B16A16Sfloat,
                    .usageFlags = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst,
                    .debugName  = fmt::format("HybridHair_Resolve_Target_{}", i),
                });

                const auto descriptorWrite = RHI::DescriptorWrite()
                    .writeStorageImage(0, vk::ImageLayout::eGeneral, mShared->visibilityBuffer[i])
                    .writeStorageImage(1, vk::ImageLayout::eGeneral, mResolveRenderTargets[i]);
                mDescriptor->write(i, descriptorWrite);
            }
        }

        void createPipeline()
        {
            /* IndirectArgs */ {
                auto pipelineInfo = RHI::ComputePipelineCreateInfo()
                    .setPushConstantRange<IndirectPushConstants>(vk::ShaderStageFlagBits::eCompute)
                    .setComputeShader(Configuration::getShaderFilePath("HybridHair_PrepareIndirect.comp.spv"))
                    .setDebugName("HybridHair_PrepareIndirect_Pipeline");
                mIndirectArgsPipeline = mRHI->createComputePipeline(pipelineInfo);
            }

            /* Visibility */ {
                auto pipelineInfo = RHI::ComputePipelineCreateInfo()
                .setPushConstantRange<PushConstants>(vk::ShaderStageFlagBits::eCompute)
                .addDescriptorSetLayout(mDescriptor->getLayout())
                .setComputeShader(Configuration::getShaderFilePath("HybridHair_Visibility.comp.spv"))
                .setDebugName("HybridHair_Visibility_Pipeline");
                mPipeline = mRHI->createComputePipeline(pipelineInfo);
            }

            /* Resolve */ {
                auto pipelineInfo = RHI::ComputePipelineCreateInfo()
                    .setPushConstantRange<ResolvePushConstants>(vk::ShaderStageFlagBits::eCompute)
                    .addDescriptorSetLayout(mDescriptor->getLayout())
                    .setComputeShader(Configuration::getShaderFilePath("HybridHair_Resolve.comp.spv"))
                    .setDebugName("HybridHair_Resolve_Pipeline");
                mResolvePipeline = mRHI->createComputePipeline(pipelineInfo);
            }
        }
    };
}
