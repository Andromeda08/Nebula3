#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

constexpr uint64_t gFramesInFlight = 2;

enum class RHIFeatureLevel
{
    Basic       = 0,
    Complete    = 1,
};
NLOHMANN_JSON_SERIALIZE_ENUM(RHIFeatureLevel, {
    { RHIFeatureLevel::Basic, "Basic" },
    { RHIFeatureLevel::Complete, "Complete" }
});

namespace RHI::Platform
{
    [[nodiscard]] constexpr RHIFeatureLevel getRHIFeatureLevel() noexcept
    {
        #ifdef __APPLE__
        return RHIFeatureLevel::Basic;
        #endif
        return RHIFeatureLevel::Complete;
    }
}

struct RHIConfiguration
{
    RHIFeatureLevel featureLevel  = RHI::Platform::getRHIFeatureLevel();
    bool            debugFeatures = true;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RHIConfiguration, featureLevel, debugFeatures);
