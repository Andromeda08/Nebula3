#pragma once

// HairGeometry.hpp
// This file defines the struct that stores all point,
// attribute, strand and strandlet data for a hair model.
// ============================================================

#include <string>
#include <vector>

#include "HairData.hpp"
#include "Core/Types.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    struct HairGeometry
    {
        // Vertex count == Attribute count
        [[nodiscard]] uint32_t getVertexCount() const noexcept
        {
            return vertexCount;
        }

        // Strand count == Strand description count
        [[nodiscard]] uint32_t getStrandCount() const noexcept
        {
            return strandCount;
        }

        // [CPU] [GPU]
        std::vector<HairVertex>     vertices;
        std::vector<HairAttributes> attributes;
        std::vector<HairStrandDesc> strandDescs;

        // [CPU]
        std::vector<uint32_t>       strandVertexCounts;
        std::vector<HairStrand>     strands;
        std::vector<HairStrandlet>  strandlets;

        // Metadata
        uint32_t                    vertexCount;
        uint32_t                    strandCount;
        uint32_t                    strandletCount;
        uint32_t                    taskGroupSizeX;
        std::string                 name;
    };

    /**
     * Stores the offsets and data count into the global buffers for each hair model.
     * (hair vertex, attribute and strand descriptions)
     */
    struct GlobalHairInfo
    {
        uint32_t firstVertex;
        uint32_t vertexCount;

        uint32_t firstAttribute;
        uint32_t attributeCount;

        uint32_t firstStrand;
        uint32_t strandCount;
    };

    class HairModelSystem
    {
    public:
        explicit HairModelSystem(const SPtr<RHI::VulkanRHI>& rhi);

        [[nodiscard]] uint32_t addHairGeometry(const HairGeometry& hairGeometry);

        [[nodiscard]] const HairGeometry& getHairGeometry(const uint32_t i) const noexcept;

        void createBuffers();

    private:
        friend class ClassicHairRenderer;
        friend class HairRenderer;

        SPtr<RHI::VulkanRHI>        mRHI;

        std::vector<HairGeometry>   mHairGeometries;

        uint32_t                    mVertexCount = 0;
        uint32_t                    mStrandCount = 0;
        std::vector<GlobalHairInfo> mHairInfos;

        SPtr<RHI::Buffer>           mHairVertices;
        SPtr<RHI::Buffer>           mHairAttributes;
        SPtr<RHI::Buffer>           mStrandDescriptions;
        SPtr<RHI::Buffer>           mGlobalHairInfo;
    };
}
