#pragma once

namespace platform
{
    #ifdef __APPLE__
    constexpr bool isApple = true;
    #else
    constexpr bool isApple = false;
    #endif
}
