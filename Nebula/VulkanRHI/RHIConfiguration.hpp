#pragma once

#include <nlohmann/json.hpp>

#include "Common.hpp"

struct RHIConfiguration
{
    RHI::FeatureLevel featureLevel  = RHI::Platform::getRHIFeatureLevel(RHI::Backend::Vulkan);
    bool              debugFeatures = true;
};

NLOHMANN_JSON_SERIALIZE_ENUM(RHI::FeatureLevel, {
    { RHI::FeatureLevel::Basic, "Basic" },
    { RHI::FeatureLevel::Complete, "Complete" }
});

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RHIConfiguration, featureLevel, debugFeatures);
