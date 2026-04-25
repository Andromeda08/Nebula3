#pragma once

#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace nbl
{
    /**
     * Light Type
     */
    enum class LightType : int32_t
    {
        Point       = 0,
        Directional = 1,
    };

    // LightType Enum -> String
    [[nodiscard]] constexpr std::string toString(const LightType lightType) noexcept
    {
        using enum LightType;
        switch (lightType)
        {
            case Point:         return "Point";
            case Directional:   return "Directional";
        }
        std::unreachable();
    }

    // Get a list of all light types.
    [[nodiscard]] const std::array<LightType, 2>& getLightTypes();

    /**
     * GPU-side representation of a Light
     */
    struct GPULightData
    {
        glm::vec3   vector;
        int32_t     lightType;

        glm::vec3   color;
        float       intensity;

        int32_t     isEnabled;
        int32_t     castsShadows;
        float       radius;
        int32_t     _pad0;
    };

    /**
     * CPU-side representation of a Light
     */
    struct Light
    {
        /**
         * Purpose varies by LightType
         * Point       -> Position
         * Directional -> Direction
         */
        glm::vec3   vector          = glm::vec3(5.0f, 25.0f, 5.0f);
        glm::vec3   color           = glm::vec3(1.0f);
        float       intensity       = 500.0f;

        // Flags
        bool        isEnabled       = true;
        bool        castsShadows    = true;

        // Point light properties
        float       radius          = 10.0f;

        LightType   type            = LightType::Directional;
        std::string name            = "Light";

        [[nodiscard]] GPULightData toGPU() const
        {
            return {
                .vector         = vector,
                .lightType      = std::to_underlying(type),
                .color          = color,
                .intensity      = intensity,
                .isEnabled      = isEnabled    ? 1 : 0,
                .castsShadows   = castsShadows ? 1 : 0,
                .radius         = radius,
                ._pad0          = 0,
            };
        }
    };
}
