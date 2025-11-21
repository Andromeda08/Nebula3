#pragma once

#include <format>
#include <string>

#include "Types.hpp"
#include "VulkanRHI/RHIConfiguration.hpp"

constexpr std::string toString(const bool value) noexcept
{
    return value ? "yes" : "no";
}

inline std::string toString(const Size2D& size2D) noexcept
{
    return std::format("[w={}, h={}]", size2D.width, size2D.height);
}

inline std::string toString(const RHIFeatureLevel featureLevel) noexcept
{
    if (featureLevel == RHIFeatureLevel::Basic)
    {
        return "Basic";
    }
    return "Complete";
}
