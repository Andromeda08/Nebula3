#pragma once

#include <glm/glm.hpp>

namespace nbl
{
    struct Meshlet
    {
        uint32_t        firstVertex;
        uint32_t        vertexCount;
        uint32_t        firstTriangle;
        uint32_t        triangleCount;

        // Bounding Sphere
        glm::vec3       center;
        float           radius;

        // Backface Cone Culling
        glm::vec3       coneAxis;
        float           coneCutoff;
    };
}
