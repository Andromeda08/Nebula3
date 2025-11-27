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

struct Range
{
    int32_t start = 0;
    int32_t end   = 0;

    Range() = default;

    Range(const int32_t a, const int32_t b) : start(a), end(b) {}

    // Merge two ranges.
    [[nodiscard]] static Range merge(const Range& a, const Range& b) noexcept
    {
        return { std::min(a.start, b.start), std::max(a.end, b.end) };
    }

    // Grows the current range endpoints by some other range.
    void grow(const Range& other) noexcept
    {
        start = std::min(start, other.start);
        end   = std::max(end,   other.end);
    }

    // Check whether two ranges overlap or not. (endpoint inclusive)
    [[nodiscard]] bool overlaps(const Range& other) const noexcept
    {
        return std::max(start, other.start) <= std::min(end, other.end);
    }

    // Check whether a range is included within the current one. (endpoint inclusive)
    [[nodiscard]] bool contains(const Range& other) const noexcept
    {
        return start <= other.start && other.end <= end;
    }

    // Check whether a value is contained within the current range. (endpoint inclusive)
    [[nodiscard]] bool includes(const int32_t value) const noexcept
    {
        return start <= value && value <= end;
    }

    // Get the length of the Range.
    [[nodiscard]] int32_t getLength() const noexcept
    {
        return end - start;
    }

    [[nodiscard]] auto toString() const noexcept -> std::string
    {
        return std::format("[{}, {}]", start, end);
    }
};
