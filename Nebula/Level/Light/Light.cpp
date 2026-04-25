#include "Light.hpp"

namespace nbl
{
    const std::array<LightType, 2>& getLightTypes()
    {
        static constexpr std::array storage = {
            LightType::Point, LightType::Directional
        };
        return storage;
    }
}