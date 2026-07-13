#pragma once

#include <vector>

#include "Core/Types.hpp"
#include "Level/Geometry/Vertex.hpp"
#include "Math/BoundingBox.hpp"
#include "VulkanRHI/Rendering/VertexTraits.hpp"

namespace nbl
{
    /**
     * Geometry Class
     * Owns vertex and index data and stores the original bounding box.
     */
    class [[nodiscard]] Geometry
    {
    public:
        Geometry(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices, std::string name);

        const BoundingBox& getBoundingBox() const noexcept;

        float getTriangleArea(size_t triIndex) const;

        const std::vector<Vertex>& getVertices() const noexcept;
        uint32_t getVertexCount() const noexcept;

        const std::vector<uint32_t>& getIndices() const noexcept;
        uint32_t getIndexCount() const noexcept;

        uint32_t getTriangleCount() const noexcept;

        const std::string& getName() const noexcept;

        void generateTangents();

    private:
        void computeBoundingBox();

        std::vector<Vertex>     mVertices       = {};
        uint32_t                mVertexCount    = 0;
        std::vector<uint32_t>   mIndices        = {};
        uint32_t                mIndexCount     = 0;
        std::string             mName           = "Unknown";

        // Computed Values
        uint32_t                mTriangleCount  = 0;
        BoundingBox             mBoundingBox    = {};
    };

    /**
     * Cube Geometry Utility
     */
    class Cube
    {
    public:
        [[nodiscard]] static SPtr<Geometry> createGeometry(std::string name = "Cube", float sideLength = 1.0f);

    private:
        [[nodiscard]] static std::vector<Vertex>   generateVertices(float sideLength);
        [[nodiscard]] static std::vector<uint32_t> generateIndices();
    };

    /**
     * Sphere Geometry Utility
     */
    class Sphere
    {
    public:
        [[nodiscard]] static SPtr<Geometry> createGeometry(std::string name = "Sphere", float radius = 1.0f, int32_t resolution = 60);

    private:
        [[nodiscard]] static std::vector<Vertex>   generateVertices(int32_t stackCount, int32_t sectorCount, float radius);
        [[nodiscard]] static std::vector<uint32_t> generateIndices(int32_t stackCount, int32_t sectorCount);
    };
}
