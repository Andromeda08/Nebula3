#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include "../Math/BoundingBox.hpp"
#include "Meshlet.hpp"
#include "../VertexTypes.hpp"

namespace nbl
{
    struct GeometryCreateInfo
    {
        std::vector<Vertex>           vertices;
        std::vector<VertexAttributes> attributes;
        std::vector<IndexType>        indices;
        std::string                   name;
    };

    class Geometry
    {
    public:
        // Meshlet generation configuration
        static constexpr uint32_t sMeshletMaxVertices  = 64;
        static constexpr uint32_t sMeshletMaxTriangles = 126;
        static constexpr float    sMeshletConeWeight   = 0.25f;

        explicit Geometry(const GeometryCreateInfo& createInfo);

        [[nodiscard]] uint32_t getIndexCount() const noexcept
        {
            return static_cast<uint32_t>(mIndices.size());
        }

        [[nodiscard]] const std::vector<IndexType>& getIndices() const noexcept
        {
            return mIndices;
        }

        [[nodiscard]] const std::vector<IndexType>& getShadowIndices() const noexcept
        {
            return mShadowIndices;
        }

        [[nodiscard]] uint32_t getVertexCount() const noexcept
        {
            return static_cast<uint32_t>(mVertices.size());
        }

        [[nodiscard]] const std::vector<Vertex>& getVertices() const noexcept
        {
            return mVertices;
        }

        [[nodiscard]] const std::vector<VertexAttributes>& getVertexAttributes() const noexcept
        {
            return mAttributes;
        }

        [[nodiscard]] uint32_t getMeshletCount() const noexcept
        {
            return static_cast<uint32_t>(mMeshlets.size());
        }

        [[nodiscard]] const std::vector<Meshlet>& getMeshlets() const noexcept
        {
            return mMeshlets;
        }

        [[nodiscard]] const std::vector<uint32_t>& getMeshletVertices() const noexcept
        {
            return mMeshletVertices;
        }

        [[nodiscard]] const std::vector<uint8_t>& getMeshletTriangles() const noexcept
        {
            return mMeshletTriangles;
        }

        [[nodiscard]] uint32_t getMeshletVertexCount() const noexcept
        {
            return static_cast<uint32_t>(mMeshletVertices.size());
        }

        [[nodiscard]] uint32_t getMeshletTriangleCount() const noexcept
        {
            return static_cast<uint32_t>(mMeshletTriangles.size());
        }

        [[nodiscard]] const BoundingBox& getBoundingBox() const noexcept
        {
            return mBoundingBox;
        }

        [[nodiscard]] const std::string& getName() const noexcept
        {
            return mName;
        }

    private:
        // Runs meshopt_optimizeVertexCache and meshopt_optimizeOverdraw
        void optimizeGeometry();

        void computeBoundingBox();

        void computeShadowIndices();

        // Generate and optimize meshlets with bounds
        void generateMeshlets();

        std::string                     mName;

        std::vector<Vertex>             mVertices;
        std::vector<VertexAttributes>   mAttributes;
        std::vector<IndexType>          mIndices;

        BoundingBox                     mBoundingBox;

        std::vector<IndexType>          mShadowIndices;

        std::vector<Meshlet>            mMeshlets;
        std::vector<uint32_t>           mMeshletVertices;
        std::vector<uint8_t>            mMeshletTriangles;
    };
}
