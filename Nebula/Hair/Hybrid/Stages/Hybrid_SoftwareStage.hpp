#pragma once

#include <string>
#include <glm/glm.hpp>

#include "Hair/Hybrid/Shared.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class Hybrid_SoftwareStage
    {
        struct IndirectPushConstants
        {
            uint64_t smallTriangleCountBuffer;
            uint64_t indirectArgsBuffer;
            uint32_t groupSize;
            uint32_t maxGroups;
        };

        struct RasterizerPushConstants
        {
            uint64_t        cameraBuffer;
            uint64_t        lightsBuffer;
            uint64_t        trianglesBuffer;
            uint64_t        triangleCountBuffer;
            glm::vec2       viewportSize;
            MarschnerBSDF   bsdf;
        };

    public:
        Hybrid_SoftwareStage(const SPtr<RHI::VulkanRHI>& rhi, HairShared* pShared);

        void execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const HairRenderer_BDAs& buffers);

    private:
        const std::string                mLabel             = "SoftwareStage";
        const std::string                mLabelIndirectArgs = "ComputeIndirectArgs";
        const std::string                mLabelRasterizer   = "Rasterizer";

        SPtr<RHI::VulkanRHI>             mRHI;
        HairShared*                      mShared;

        // Indirect Args Pipeline
        // ================================================
        PerFrameArray<SPtr<RHI::Buffer>> mIndirectArgsBuffer;
        UPtr<RHI::ComputePipeline2>      mIndirectArgsPipeline;

        // Rasterizer Pipeline
        // ================================================
        SPtr<RHI::Descriptor>            mDescriptor;
        UPtr<RHI::ComputePipeline2>      mPipeline;

    };
}