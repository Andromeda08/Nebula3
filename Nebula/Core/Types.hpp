#pragma once

#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>

template <class T>
using UPtr = std::unique_ptr<T>;

template <class T>
using SPtr = std::shared_ptr<T>;

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
