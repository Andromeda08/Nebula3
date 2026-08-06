#pragma once

#include <glm/glm.hpp>

namespace nbl
{
    enum class MarschnerDebugMode : int32_t
    {
        FinalColor          = 0,
        LongScattering      = 1,
        AzimuthalScattering = 2,
        CombinedScattering  = 3,
        Ambient             = 4,
        Diffuse             = 5,
        Marschner           = 6,
    };

    inline const char* toString(const MarschnerDebugMode e)
    {
        using enum MarschnerDebugMode;
        switch (e)
        {
            case FinalColor:          return "Color (None)";
            case LongScattering:      return "LongScattering";
            case AzimuthalScattering: return "AzimuthalScattering";
            case CombinedScattering:  return "CombinedScattering";
            case Ambient:             return "Ambient";
            case Diffuse:             return "Diffuse";
            case Marschner:           return "Marschner";
            default:                  return "unknown";
        }
    }

    struct MarschnerBSDF
    {
        glm::vec3 diffuseTint    = glm::vec3(0.32549f, 0.23921f, 0.20784f);
        glm::vec3 specularTint   = glm::vec3(0.41568f, 0.30588f, 0.21960f);

        glm::vec3 absorption     = glm::vec3(0.4, 0.2, 0.05);

        float     roughness      = glm::radians(6.0f);
        float     azimuthalWidth = 0.3f;
        float     shiftR         = glm::radians(-4.5f);
        float     shiftTT        = 0.0f;
        float     shiftTRT       = glm::radians(4.5f);
        float     scaleR         = 1.0f;
        float     scaleTT        = 1.0f;
        float     scaleTRT       = 0.5f;
    };
}
