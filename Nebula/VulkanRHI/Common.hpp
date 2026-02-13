#pragma once

#include <cstdint>

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
    constexpr uint64_t gFramesInFlight = 2;

    // TODO: completely replace gFramesInFlight
    constexpr uint64_t gMaxConcurrentFrames = gFramesInFlight;
}

// RHI Common Types
// ============================
namespace RHI
{
    enum class Backend
    {
        Vulkan,
    };

    enum class RHIFeatureLevel
    {
        Basic       = 0,
        Complete    = 1,
    };

    // TODO: alias to enum class
    using FeatureLevel = RHIFeatureLevel;

    enum class QueueType
    {
        Graphics,
        Compute,
        Transfer,
    };
}

// RHI::Platform
// ============================
namespace RHI::Platform
{
    // Get the preferred backend for the current Platform
    [[nodiscard]] constexpr Backend getPreferredBackend() noexcept
    {
        return Backend::Vulkan;
    }

    // Get the RHI feature level for the current Platform given a specific Backend
    [[nodiscard]] inline FeatureLevel getRHIFeatureLevel(const Backend backend) noexcept
    {
        if (isApple && backend == Backend::Vulkan)
        {
            return FeatureLevel::Basic;
        }
        return FeatureLevel::Complete;
    }
}
