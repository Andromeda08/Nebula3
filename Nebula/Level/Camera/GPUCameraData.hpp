#pragma once

#include <glm/glm.hpp>

namespace nbl
{
    struct GPUCameraData
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
        glm::vec4 eye;
        glm::vec4 frustumPlanes[6];
        float     nearPlane;
        float     farPlane;
    };
}
