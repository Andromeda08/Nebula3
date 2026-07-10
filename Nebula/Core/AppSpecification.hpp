#pragma once

#include <string>
#include <nlohmann/json.hpp>

#include "Types.hpp"

struct AppSpecification
{
    Size2D      windowSize  = Resolution::w1920h1080();
    std::string windowTitle = "Nebula Engine";
    std::string appName     = "Nebula Application";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AppSpecification, windowSize, windowTitle, appName);