#pragma once

#include <string_view>
#include "Core/Types.hpp"

struct WindowCreateInfo
{
    Size2D           size;
    std::string_view title;
    bool             isResizable = false;
};
