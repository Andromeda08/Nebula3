#pragma once

#include <string>
#include <nlohmann/json.hpp>

#include "Types.hpp"

constexpr bool gEnableUserInterface = true;

namespace detail
{
    [[nodiscard]] Size2D getDefaultResolution() noexcept;
}

struct AppSpecification
{
    Size2D      windowSize  = detail::getDefaultResolution();
    std::string windowTitle = "Nebula Engine";
    std::string appName     = "Nebula Application";
    std::string shadersDir  = "Resources/Shaders/bin";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AppSpecification, windowSize, windowTitle, appName, shadersDir);