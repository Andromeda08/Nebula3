#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "Core/Types.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    struct TestTriangle
    {
        glm::vec3   v0;
        glm::vec3   v1;
        glm::vec3   v2;
        uint32_t    id; // primitive id
    };

    class SoftwareRasterizer
    {
        struct PushConstants
        {
            // Buffer References
            uint64_t trianglesBuffer;

            // Resolution
            uint32_t width;
            uint32_t height;

            // Scissor
            uint32_t sMinX, sMinY, sMaxX, sMaxY;

            // Triangle Count
            uint32_t triangleCount;
        };

        struct ResolvePushConstants
        {
            uint64_t colorsBuffer;
            uint32_t width;
            uint32_t height;
        };

    public:
        explicit SoftwareRasterizer(const SPtr<RHI::VulkanRHI>& rhi);

        void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const;

    private:
        SPtr<RHI::VulkanRHI>            mRHI;

        static constexpr uint32_t sInvalidPrimitiveId = std::numeric_limits<uint32_t>::max();

        // Visibility Buffer Pipeline
        // ================================================
        vk::Extent2D                    mRenderResolution;
        PerFrameArray<SPtr<RHI::Image>> mRenderTargets;
        SPtr<RHI::Descriptor>           mDescriptor;
        UPtr<RHI::ComputePipeline>      mPipeline;

        // Resolve Pipeline
        // ================================================
        PerFrameArray<SPtr<RHI::Image>> mResolveRenderTargets;
        SPtr<RHI::Descriptor>           mResolveDescriptor;
        UPtr<RHI::ComputePipeline>      mResolvePipeline;

        void createResources();

        void createPipeline();

        // Test Triangles & Colors for IDs
        // ================================================
        std::vector<TestTriangle>       mTestData;
        SPtr<RHI::Buffer>               mTestDataBuffer;
        uint32_t                        mTriangleCount = 0;

        std::vector<glm::vec3>          mDebugColors;
        SPtr<RHI::Buffer>               mDebugColorsBuffer;

        void createTestTriangleData();

        void createColors();
    };
}
