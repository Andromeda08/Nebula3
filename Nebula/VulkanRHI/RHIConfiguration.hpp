#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

constexpr uint64_t gFramesInFlight = 2;

enum class RHIFeatureLevel
{
    Basic,
    Complete,
};
NLOHMANN_JSON_SERIALIZE_ENUM(RHIFeatureLevel, {
    { RHIFeatureLevel::Basic, "Basic" },
    { RHIFeatureLevel::Complete, "Complete" }
});

struct RHIConfiguration
{
    RHIFeatureLevel featureLevel  = RHIFeatureLevel::Complete;
    bool            debugFeatures = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RHIConfiguration, featureLevel, debugFeatures);
