#pragma once

#include <nlohmann/json.hpp>

struct SceneConfiguration
{
    std::string texturesDir = "Resources/Textures";
    std::string moleculesDir = "Resources/CIFFiles";
    std::string molecule = "5XUY.cif";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SceneConfiguration, texturesDir, moleculesDir, molecule);
