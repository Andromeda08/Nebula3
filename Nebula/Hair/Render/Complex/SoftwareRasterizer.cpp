#include "SoftwareRasterizer.hpp"

#include "Core/Random.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace nbl
{
    SoftwareRasterizer::SoftwareRasterizer(const SPtr<RHI::VulkanRHI>& rhi): mRHI(rhi)
    {
        createTestTriangleData();
        createColors();

        mRenderResolution = vk::Extent2D { 256, 256 };

        createResources();
        createPipeline();
    }

    void SoftwareRasterizer::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const
    {
        pCommandList->beginLabel("Software_Rasterizer");

        const auto& visibilityTarget = mRenderTargets[frameData.currentFrame];
        const auto& colorTarget      = mResolveRenderTargets[frameData.currentFrame];

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

        // Run visibility pipeline
        // ================================================
        pCommandList->beginLabel("Visibility");
        {
            const auto pushConstants = PushConstants {
                .trianglesBuffer = mTestDataBuffer->getAddress(),
                .width           = mRenderResolution.width,
                .height          = mRenderResolution.height,
                .sMinX           = 0,
                .sMinY           = 0,
                .sMaxX           = mRenderResolution.width  - 1,
                .sMaxY           = mRenderResolution.height - 1,
                .triangleCount   = mTriangleCount,
            };

            RHI::Barrier()
                .addBarrier(visibilityTarget->getBarrier(RHI::ImageUsage::StorageImage))
                .addBarrier(mTestDataBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::StorageRead))
                .insert(pCommandList);

            mPipeline->bind(pCommandList);
            mPipeline->pushConstants(pCommandList, &pushConstants);
            mPipeline->bindDescriptorSet(pCommandList, mDescriptor->getSet(frameData.currentFrame));

            constexpr uint32_t groupSize = 64;
            const     uint32_t groupX    = (mTriangleCount + groupSize - 1) / groupSize;
            mPipeline->dispatch(pCommandList, groupX, 1, 1);
        }
        pCommandList->endLabel();

        // Run resolve pipeline
        // ================================================
        pCommandList->beginLabel("Resolve");
        {
            const auto pushConstants = ResolvePushConstants {
                .colorsBuffer = mDebugColorsBuffer->getAddress(),
                .width        = mRenderResolution.width,
                .height       = mRenderResolution.height,
            };

            RHI::Barrier()
               .addBarrier(visibilityTarget->getBarrier(RHI::ImageUsage::StorageImage))
               .addBarrier(colorTarget->getBarrier(RHI::ImageUsage::StorageImage))
               .addBarrier(mTestDataBuffer->getBarrier(RHI::BufferUsage::StorageRead, RHI::BufferUsage::StorageRead))
               .addBarrier(mDebugColorsBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::StorageRead))
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

    void SoftwareRasterizer::createResources()
    {
        mDescriptor = mRHI->createDescriptor({
            .bindings  = {
                // Visibility Buffer
            { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
                // Color Target
            { 1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
            },
            .setCount  = RHI::gFramesInFlight,
            .debugName = "swr_test_descriptor",
        });

        for (size_t i = 0; i < mRenderTargets.size(); i++)
        {
            mRenderTargets[i] = mRHI->createImage({
                .extent     = mRenderResolution,
                .format     = vk::Format::eR64Uint,
                .usageFlags = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst,
                .debugName  = fmt::format("swr_test_target_visibility_{}", i),
            });

            mResolveRenderTargets[i] = mRHI->createImage({
                .extent     = mRenderResolution,
                .format     = vk::Format::eR16G16B16A16Sfloat,
                .usageFlags = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst,
                .debugName  = fmt::format("swr_test_target_color_{}", i),
            });

            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeStorageImage(0, vk::ImageLayout::eGeneral, mRenderTargets[i])
                .writeStorageImage(1, vk::ImageLayout::eGeneral, mResolveRenderTargets[i]);
            mDescriptor->write(i, descriptorWrite);
        }
    }

    void SoftwareRasterizer::createPipeline()
    {
        /* Visibility */ {
            auto pipelineInfo = RHI::ComputePipelineCreateInfo()
            .setPushConstantRange<PushConstants>(vk::ShaderStageFlagBits::eCompute)
            .addDescriptorSetLayout(mDescriptor->getLayout())
            .setComputeShader(Configuration::getShaderFilePath("Rasterizer.comp.spv"))
            .setDebugName("SoftwareRasterizer_Visibility_Pipeline");
            mPipeline = mRHI->createComputePipeline(pipelineInfo);
        }

        /* Resolve */ {
            auto pipelineInfo = RHI::ComputePipelineCreateInfo()
                .setPushConstantRange<ResolvePushConstants>(vk::ShaderStageFlagBits::eCompute)
                .addDescriptorSetLayout(mDescriptor->getLayout())
                .setComputeShader(Configuration::getShaderFilePath("Resolve.comp.spv"))
                .setDebugName("SoftwareRasterizer_Resolve_Pipeline");
            mResolvePipeline = mRHI->createComputePipeline(pipelineInfo);
        }
    }

    void SoftwareRasterizer::createTestTriangleData()
    {
        // CCW winded triangle
        const auto t1_ccw = TestTriangle {
            .v0 = { 20.0f, 50.0f, 0.1f },
            .v1 = { 50.0f, 60.0f, 0.1f },
            .v2 = { 30.0f, 90.0f, 0.1f },
            .id = static_cast<uint32_t>(mTestData.size()),
        };
        mTestData.push_back(t1_ccw);

        // CW winded triangle
        const auto t2_cw = TestTriangle {
            .v0 = {  80.0f, 40.0f, 0.2f },
            .v1 = { 140.0f, 80.0f, 0.2f },
            .v2 = { 100.0f, 20.0f, 0.2f },
            .id = static_cast<uint32_t>(mTestData.size()),
        };
        mTestData.push_back(t2_cw);

        mTriangleCount = static_cast<uint32_t>(mTestData.size());

        // Upload to GPU
        const uint64_t bufferSize = mTestData.size() * sizeof(TestTriangle);
        mTestDataBuffer           = mRHI->createBuffer({
            .size  = bufferSize,
            .type  = RHI::BufferType::Storage,
            .label = "swr_test_triangles",
        });
        mRHI->immediate_uploadToBuffer(mTestDataBuffer.get(), mTestData.data(), bufferSize);
    }

    void SoftwareRasterizer::createColors()
    {
        // Generate a color for each triangle
        mDebugColors.reserve(mTriangleCount);
        for (size_t i = 0; i < mTriangleCount; i++)
        {
            const auto color = Random::getColor();
            mDebugColors.push_back(glm::xyz(color));
        }

        // Upload to GPU
        const uint64_t bufferSize = mDebugColors.size() * sizeof(glm::vec3);
        mDebugColorsBuffer        = mRHI->createBuffer({
            .size  = bufferSize,
            .type  = RHI::BufferType::Storage,
            .label = "swr_debug_colors",
        });
        mRHI->immediate_uploadToBuffer(mDebugColorsBuffer.get(), mDebugColors.data(), bufferSize);
    }
}
