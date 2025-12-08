#pragma once

#include <glm/glm.hpp>

struct LightParameters
{
    glm::vec4 position  = {};
    glm::vec4 color     = {};
    float     intensity = 10000.0f;
    int32_t   active    = 0;
    int32_t   _pad0     = 0;
    int32_t   _pad1     = 0;
};
