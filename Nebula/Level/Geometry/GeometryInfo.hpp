#pragma once

#include <limits>
#include "Vertex.hpp"

namespace nbl
{
    /**
     * CPU and GPU-side descriptor for geometries
     */
    struct GeometryInfo
    {
        int32_t   geometryIndex = -1;
        uint32_t  triangleCount = 0;

        // VertexBuffer Region
        uint32_t  firstVertex   = std::numeric_limits<uint32_t>::max();
        uint32_t  vertexCount   = 0;

        // IndexBuffer Region
        uint32_t  firstIndex    = std::numeric_limits<uint32_t>::max();
        uint32_t  indexCount    = 0;

        /**
         * @return Size of geometry vertex data in bytes
         */
        [[nodiscard]] uint64_t getVertexSize() const noexcept
        {
            return vertexCount * sizeof(Vertex);
        }

        /**
         * @return Size of geometry index data in bytes
         */
        [[nodiscard]] uint64_t getIndexSize() const noexcept
        {
            return indexCount * sizeof(uint32_t);
        }
    };
}
