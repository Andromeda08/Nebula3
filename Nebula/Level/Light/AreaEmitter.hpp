#pragma once

#include <glm/glm.hpp>

namespace nbl
{
    struct AreaEmitter
    {
        uint32_t  instanceIndex;
        int32_t   geometryIndex;
        uint32_t  cdfOffset;
        uint32_t  triCount;
        float     totalWeight;
        glm::vec3 radiance;
    };
}
