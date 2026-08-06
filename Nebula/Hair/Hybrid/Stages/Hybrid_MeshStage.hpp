#pragma once

#include <string>
#include <glm/glm.hpp>

#include "Hair/Hybrid/Shared.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class Hybrid_MeshStage
    {
        struct PushConstants
        {
            glm::mat4       model;

            // Buffer references
            uint64_t        vertexBuffer;
            uint64_t        attributesBuffer;
            uint64_t        strandDescriptionBuffer;
            uint64_t        cameraBuffer;
            uint64_t        lightsBuffer;
            uint64_t        trianglesBuffer;

            // Hair model specific global buffer offsets
            uint32_t        firstVertex;
            uint32_t        vertexCount;
            uint32_t        firstStrand;
            uint32_t        strandCount;

            // Path choosing
            uint64_t        smallTriangleCounterBuffer;
            uint32_t        maxSmallTriangles;
            float           smallTriangleThreshold;
            glm::vec2       viewport;

            int32_t         isHybrid;
            MarschnerBSDF   bsdf;
        };
    public:
        Hybrid_MeshStage(const SPtr<RHI::VulkanRHI>& rhi, HairShared* pShared);

        void execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const HairRenderer_BDAs& buffers);

    private:
        const std::string               mLabel         = "MeshStage";
        const std::string               mLabelPreamble = "Preamble";
        const std::string               mLabelMesh     = "MeshShader";

        SPtr<RHI::VulkanRHI>            mRHI;
        HairShared*                     mShared;
        SPtr<RHI::GraphicsPipeline2>    mPipeline;
    };
}