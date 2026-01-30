#pragma once

#include <format>
#include <sstream>
#include <string>
#include <glm/glm.hpp>

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

namespace fmt
{
    /**
     * Convert any glm vector to a string with the format [{}, {}, ...]
     * @param vec
     */
    template<glm::length_t L, typename T, glm::qualifier Q>
    [[nodiscard]] std::string vec(const glm::vec<L, T, Q>& vec) noexcept
    {
        std::stringstream sstr;
        sstr << "[";
        for (auto i = 0; i < L; i++)
        {
            if (i != 0)
            {
                sstr << ", ";
            }
            sstr << vec[i];
        }
        sstr << "]";
        return sstr.str();
    }
}
