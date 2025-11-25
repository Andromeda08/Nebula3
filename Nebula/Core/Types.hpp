#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>

#include "VulkanRHI/RHIConfiguration.hpp"

template <class T>
using PerFrameArray = std::array<T, gFramesInFlight>;

template <class T>
using UPtr = std::unique_ptr<T>;

template <class T, class... Args>
constexpr UPtr<T> makeUnique(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <class T>
using SPtr = std::shared_ptr<T>;

template <class T, class... Args>
constexpr UPtr<T> makeShared(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

struct Size2D
{
    uint32_t width  = 0u;
    uint32_t height = 0u;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Size2D, width, height);

namespace Resolution
{
    #define DEF_RESOLUTION(WIDTH, HEIGHT) constexpr Size2D w##WIDTH##h##HEIGHT() noexcept { return { WIDTH, HEIGHT }; }

    DEF_RESOLUTION(1280, 720);
    DEF_RESOLUTION(1600, 900);
    DEF_RESOLUTION(1920, 1080);
    DEF_RESOLUTION(2560, 1440);

    #undef DEF_RESOLUTION
}
