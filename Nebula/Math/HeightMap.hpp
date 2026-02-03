#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include <glm/glm.hpp>

/**
 * HeightMap generator class
 * @note By default, sample  returns positions in their original [0, Size] X and Z range,
 * if "centered" is true, the range is [-(Size / 2), Size / 2]!
 */
class HeightMap
{
public:
    struct Control
    {
        std::size_t   size      = 256;
        std::uint32_t maxHeight = 24;
        std::uint32_t flatness  = 96;
        bool          centered  = false;
    };

    explicit HeightMap(const Control& control);

    // Indexed always on [0, Size]
    [[nodiscard]] std::optional<int32_t> at(std::size_t u, std::size_t v) const noexcept;

    // Indexed always on [0, Size]
    [[nodiscard]] std::optional<int32_t> at(const glm::ivec2& uv) const noexcept;

    // Returns a random point the height map ([X, Z] axes).
    [[nodiscard]] glm::ivec2 randomPoint() const noexcept;

    // Returns a random (height) sample from the height map
    [[nodiscard]] int32_t sampleHeight() const noexcept;

    // Returns a random position sample from the height map (height in Y).
    [[nodiscard]] glm::vec3 sample() const noexcept;

    [[nodiscard]] const std::vector<std::vector<int32_t>>& getHeightMap() const noexcept;

    [[nodiscard]] int32_t getMin() const noexcept;

    [[nodiscard]] int32_t getMax() const noexcept;

private:
    float           mHeightScalar;
    float           mFlatnessScalar;
    bool            mCentered;
    std::size_t     mSize;
    std::int32_t    mMin = 0;
    std::int32_t    mMax = 0;

    std::vector<std::vector<int32_t>> mHeightMap;
};
