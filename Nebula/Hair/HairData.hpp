#pragma once

// HairData.hpp
// This file contains all structures used for describing
// Hair geometry using vertices, strands and strandlets
// ============================================================

#include <cstdint>
#include <span>
#include <glm/glm.hpp>

namespace nbl
{
    // Workgroup size used for GPU work
    constexpr uint32_t gWorkgroupSize = 32;

    /**
     * Maximum strandlet size (ideally) matches the workgroup size.
     * Mesh shader threads this way each produce a single quad within a workgroup,
     * and a workgroup processes a strandlet.
     */
    constexpr uint32_t gHairMaxStrandletSize = gWorkgroupSize;

    /**
     * Hair Vertex Data Representation [CPU] [GPU]
     * 12 bytes
     */
    struct HairVertex
    {
        glm::vec3 position;
    };

    /**
     * Hair Vertex Attribute Data [CPU] [GPU]
     * 20 Bytes
     */
    struct HairAttributes
    {
        glm::vec3 color;        // RGB
        float     thickness;    // Thickness of the strand at this position
        float     transparency; // Transparency at this point
    };

    using StrandId = int32_t;
    /**
     * Hair Strand Data [CPU]
     * A hair strand consists of several points that are later broken down into strandlets.
     */
    struct HairStrand
    {
        StrandId              id          = 0;  // Referenced by Strandlets
        uint32_t              vertexCount = 0;  // No. vertices in the strand
        std::span<HairVertex> vertices    = {}; // Non-owning view of the vertices of the strand
    };

    /**
     * Hair Strandlet Data [CPU]
     * A hair strandlet is a segment of a maximum fixed size of a hair strand.
     */
    struct HairStrandlet
    {
        StrandId              strandId    = 0;  // Which strand this strandlet belongs to
        uint32_t              vertexCount = 0;  // No. vertices in the strandlet
        std::span<HairVertex> vertices    = {}; // Non-owning view of the vertices of the strandlet
    };

    /**
     * Struct describing a single hair strand [CPU] [GPU]
     */
    struct HairStrandDesc
    {
        StrandId strandId       = 0;
        uint32_t vertexCount    = 0;
        uint32_t strandletCount = 0;
        uint32_t firstVertex    = 0;
    };

    /**
     * Struct for passing buffer addresses to the GPU [CPU] [GPU]
     */
    struct HairBufferAddresses
    {
        uint64_t vertexBuffer     = 0;
        uint64_t strandDescBuffer = 0;
    };
}
