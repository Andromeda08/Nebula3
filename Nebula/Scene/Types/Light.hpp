#pragma once

#include <string>
#include <glm/glm.hpp>

enum class LightType
{
    Point,
    Directional,
};

[[nodiscard]] constexpr std::string_view toString(const LightType e) noexcept
{
    using enum LightType;
    switch (e)
    {
        case Point:       return "Point";
        case Directional: return "Directional";
        default:          return "Unknown";
    }
}

struct Light
{
    glm::vec3   position    = glm::vec3(0.0f, 50.0f, 0.0f);
    glm::vec3   color       = glm::vec3(1.0f);
    float       intensity   = 1500.0f;
    bool        enabled     = true;
    bool        castsShadow = true;
    LightType   type        = LightType::Point;
    std::string name        = "Light";
};
