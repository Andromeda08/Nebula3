#pragma once

#include <nlohmann/json.hpp>

struct SceneConfiguration
{
    std::string texturesDir = "Resources/Textures";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SceneConfiguration, texturesDir);
