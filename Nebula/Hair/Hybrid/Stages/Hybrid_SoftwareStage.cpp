#include "Hybrid_SoftwareStage.hpp"

#include "Hair/Hybrid/GPUScreenSpaceTriangle.hpp"

namespace nbl
{
    Hybrid_SoftwareStage::Hybrid_SoftwareStage(const SPtr<RHI::VulkanRHI>& rhi, HairShared* pShared)
    : mRHI(rhi)
    , mShared(pShared)
    {
        mDescriptor = mRHI->createDescriptor({
            .bindings  = {
                // Color Target
            { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
                // Depth Buffer
            { 1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
            },
            .setCount  = RHI::gFramesInFlight,
            .debugName = "Hair_SoftwareStage_Descriptor",
        });

        for (size_t i = 0; i < RHI::gFramesInFlight; i++)
        {
            mIndirectArgsBuffer[i] = mRHI->createBuffer({
                .size  = sizeof(uint32_t) * 3,
                .type  = RHI::BufferType::Indirect,
                .label = fmt::format("HybridHair_IndirectArgs_{}", i),
            });

            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeStorageImage(0, vk::ImageLayout::eGeneral, mShared->colorTarget[i])
                .writeStorageImage(1, vk::ImageLayout::eGeneral, mShared->depthBuffer[i]);
            mDescriptor->write(i, descriptorWrite);
        }

        /* IndirectArgs Pipeline */
        {
            auto pipelineInfo = RHI::PipelineCommon()
                .setPushConstant<IndirectPushConstants>(vk::ShaderStageFlagBits::eCompute)
                .addShader("HybridHair_PrepareIndirect.comp.spv")
                .setLabel("Hair_IndirectArgs_Pipeline");
            mIndirectArgsPipeline = mRHI->createComputePipeline2(pipelineInfo);
        }

        /* Rasterizer Pipeline */
        {
            auto pipelineInfo = RHI::PipelineCommon()
                .setPushConstant<RasterizerPushConstants>(vk::ShaderStageFlagBits::eCompute)
                .addDescriptorLayout(0, mDescriptor.get())
                .addShader("HybridHair_Forward.comp.spv")
                .setLabel("Hair_Rasterizer_Pipeline");
            mPipeline = mRHI->createComputePipeline2(pipelineInfo);
        }
    }

    void Hybrid_SoftwareStage::execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData,
        const HairRenderer_BDAs& buffers)
    {
        pCommandList->beginLabel(mLabel);

        const auto& colorTarget = mShared->colorTarget[frameData.currentFrame];
        const auto& depthBuffer = mShared->depthBuffer[frameData.currentFrame];

        const auto& trianglesBuffer = mShared->trianglesBuffer[frameData.currentFrame];
        const auto& counterBuffer   = mShared->smallTriangleCounterBuffer[frameData.currentFrame];

        const auto& indirectArgs = mIndirectArgsBuffer[frameData.currentFrame];

        // Compute indirect dispatch args
        // ================================================
        pCommandList->beginLabel(mLabelIndirectArgs);
        {
            constexpr uint32_t groupSize         = 64;
            const     uint32_t maxSmallTriangles = mShared->currentBufferSize[frameData.currentFrame] / sizeof(GPUScreenSpaceTriangle);
            const     uint32_t maxGroups         = (maxSmallTriangles + groupSize - 1) / groupSize;

            const auto pushConstants = IndirectPushConstants {
                .smallTriangleCountBuffer = counterBuffer->getAddress(),
                .indirectArgsBuffer       = indirectArgs->getAddress(),
                .groupSize                = groupSize,
                .maxGroups                = maxGroups,
            };

            RHI::Barrier()
                .addBarrier(counterBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::Compute_Read))
                .addBarrier(indirectArgs->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::Compute_Write))
                .insert(pCommandList);

            pCommandList->bindPipeline(mIndirectArgsPipeline.get());
            pCommandList->pushConstants(&pushConstants);
            pCommandList->dispatch(1, 1, 1);
        }
        pCommandList->endLabel();

        // Rasterizer pipeline
        // ================================================
        pCommandList->beginLabel(mLabelRasterizer);
        {
            const auto pushConstants = RasterizerPushConstants {
                .cameraBuffer        = buffers.cameraBuffer,
                .lightsBuffer        = buffers.lightsBuffer,
                .trianglesBuffer     = trianglesBuffer->getAddress() ,
                .triangleCountBuffer = counterBuffer->getAddress() ,
                .viewportSize        = mShared->viewportSize,
                .bsdf                = mShared->config.bsdfParams,
            };

            RHI::Barrier()
                .addBarrier(colorTarget->getBarrier(RHI::ImageUsage::StorageImage))
                .addBarrier(depthBuffer->getBarrier(RHI::ImageUsage::StorageImage))
                .addBarrier(trianglesBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::StorageRead))
                .addBarrier(counterBuffer->getBarrier(RHI::BufferUsage::Compute_Write, RHI::BufferUsage::StorageRead))
                .addBarrier(indirectArgs->getBarrier(RHI::BufferUsage::Compute_Write, RHI::BufferUsage::DrawIndirect))
                .insert(pCommandList);

            pCommandList->bindPipeline(mPipeline.get());
            pCommandList->pushConstants(&pushConstants);
            pCommandList->bindDescriptorSet(mDescriptor->getSet(frameData.currentFrame), 0);
            pCommandList->dispatchIndirect(indirectArgs.get(), 0);
        }
        pCommandList->endLabel();

        pCommandList->endLabel();
    }
}
