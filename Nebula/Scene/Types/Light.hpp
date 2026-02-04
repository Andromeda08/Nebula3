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
    glm::vec3   position    = glm::vec3(0.0f, 5.0f, 0.0f);
    glm::vec3   color       = glm::vec3(1.0f);
    float       intensity   = 10000.0f;
    bool        enabled     = true;
    LightType   type        = LightType::Point;
    std::string name        = "UnknownLight";
};

struct alignas(16) GPULightData
{
    glm::vec4 position  = {};
    glm::vec3 color     = {};
    float     intensity = 10000.0f;
    int32_t   enabled    = 0;

    [[nodiscard]] static GPULightData fromLight(const Light& light) noexcept
    {
        return {
            .position  = glm::vec4(light.position, (light.type == LightType::Point ? 1 : 0)),
            .color     = light.color,
            .intensity = light.intensity,
            .enabled   = light.enabled ? 1 : 0,
        };
    }
};
