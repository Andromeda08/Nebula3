#pragma once

#include <glm/glm.hpp>

namespace nbl
{
    struct Material
    {
        glm::vec4 solidColor   = glm::vec4(0.8f, 0.1f, 0.8f, 1.0f);
        int32_t   baseColorTex = -1;
        int32_t   normalTex    = -1;
        int32_t   specularTex  = -1;
        int32_t   _pad0;
    };
}
