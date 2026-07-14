#pragma once

#include <cstdint>
#include <string_view>

// RHI Resource ctor macro
// ============================
#pragma region "rhi_RES_CTOR"
#define rhi_RES_CTOR(T, DEVICE_T)                                               \
    T(const T&) = delete;                                                       \
    T& operator=(const T&) = delete;                                            \
    T(const T&&) = delete;                                                      \
    T& operator=(const T&&) = delete;                                           \
    explicit T(const T##CreateInfo& createInfo, const SPtr<DEVICE_T>& device);  \
    [[nodiscard]] static SPtr<T> create(const T##CreateInfo& createInfo,        \
                                        const SPtr<DEVICE_T>& device            \
    ) noexcept { return makeShared<T>(createInfo, device); }
#pragma endregion

// Platform "Detection"
// ============================
namespace RHI::Platform
{
    #ifdef __APPLE__
    constexpr bool isApple = true;
    #else
    constexpr bool isApple = false;
    #endif
}

// Constants
// ============================
namespace RHI
{
    constexpr uint64_t gFramesInFlight = 3;
}

// RHI Common Types
// ============================
namespace RHI
{
    enum class FeatureLevel
    {
        Basic       = 0,
        Complete    = 1,
        Nvidia      = 2,
    };

    [[nodiscard]] constexpr std::string_view getFeatureLevelName(const FeatureLevel level) noexcept
    {
        using enum FeatureLevel;
        switch (level)
        {
            case Basic:     return "Basic";
            case Complete:  return "Complete";
            case Nvidia:    return "Complete (Nvidia)";
        }
        return "Unknown";
    }

    enum class QueueType
    {
        Graphics,
        Compute,
        Transfer,
    };
}
