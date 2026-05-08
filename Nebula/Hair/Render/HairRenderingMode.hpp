#pragma once

#include <cstdint>

namespace nbl
{
    enum class HairRenderingMode : uint32_t
    {
        Default         = 0,
        DebugQuads      = 1,
        DebugStrands    = 2,
        DebugStrandlets = 3,
    };
}
