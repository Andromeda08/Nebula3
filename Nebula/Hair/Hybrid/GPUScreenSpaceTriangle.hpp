#pragma once

#include <glm/glm.hpp>

namespace nbl
{
    /**
     * A single screen-space triangle.
     * The mesh shader outputs small triangles in this format for the software path to consume.
     * A vertex of a triangle is stored as such: [x pixel, y pixel, depth]
     */
    struct GPUScreenSpaceTriangle
    {
        glm::vec3 v0, v1, v2;
        glm::vec3 w0, w1, w2;
        glm::vec3 tangent;
        glm::vec3 color;
        uint32_t  primitiveId;
    };
}
