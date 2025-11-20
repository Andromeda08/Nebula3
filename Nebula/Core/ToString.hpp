#pragma once

#include <format>
#include <string>

#include "Types.hpp"

inline std::string toString(const Size2D& size2D) noexcept
{
    return std::format("[w={}, h={}]", size2D.width, size2D.height);
}
