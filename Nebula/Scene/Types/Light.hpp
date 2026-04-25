#pragma once

#include <string>
#include <glm/glm.hpp>

struct alignas(16) GPULightData
{
    glm::vec3   vector;
    int32_t     lightType;

    glm::vec3   color;
    float       intensity;

    float       radius;
    int32_t     isEnabled;
    int32_t     castsShadow;
    int32_t     _p0;
};
static_assert(sizeof(GPULightData) % 16 == 0, "Size of GPULightData must be a multiple of 16");

enum class LightType : int32_t
{
    Point       = 0,
    Directional = 1,
};

[[nodiscard]] constexpr std::string toString(const LightType e) noexcept
{
    using enum LightType;
    switch (e)
    {
        case Point:       return "Point";
        case Directional: return "Directional";
        default:          return "Unknown";
    }
}

inline std::vector<std::string> getLightTypes()
{
    return { toString(LightType::Point), toString(LightType::Directional) };
}

struct Light
{
    glm::vec3   vector      = glm::vec3(0.0f, 50.0f, 0.0f);
    glm::vec3   color       = glm::vec3(1.0f);
    float       intensity   = 250.0f;

    // Light Flags
    bool        isEnabled   = true;
    bool        castsShadow = true;

    // Point Lights
    float       radius      = 10.0f;

    LightType   type        = LightType::Point;
    std::string name        = "Light";

    /**
     * Convert CPU-side representation to GPU-side data
     * @return GPU Data
     */
    [[nodiscard]] GPULightData toGpuData() const
    {
        return {
            .vector      = vector,
            .lightType   = static_cast<int32_t>(type),
            .color       = color,
            .intensity   = intensity,
            .radius      = radius,
            .isEnabled   = isEnabled ? 1 : 0,
            .castsShadow = castsShadow ? 1 : 0,
        };
    }
};
