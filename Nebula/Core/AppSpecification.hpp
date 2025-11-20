#pragma once

#include <string>
#include <nlohmann/json.hpp>

#include "Types.hpp"

constexpr bool gEnableUserInterface = true;

struct AppSpecification
{
    Size2D      windowSize  = Resolution::w1280h720();
    std::string windowTitle = "Unknown Window";
    std::string appName     = "Unknown Application";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AppSpecification, windowSize, windowTitle, appName);