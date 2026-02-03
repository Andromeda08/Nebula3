#include "HeightMap.hpp"

#include <glm/gtc/noise.hpp>

#include "Core/Random.hpp"

HeightMap::HeightMap(const Control& control)
{
    mSize           = control.size;
    mHeightScalar   = static_cast<float>(control.maxHeight);
    mFlatnessScalar = static_cast<float>(control.flatness);
    mCentered       = control.centered;

    mHeightMap.resize(mSize);
    for (std::size_t y = 0; y < mSize; y++)
    {
        std::vector<int32_t> row(mSize);
        for (std::size_t x = 0; x < mSize; x++)
        {
            const auto p = glm::vec2(static_cast<float>(y), static_cast<float>(x)) / mFlatnessScalar;
            const auto v = glm::simplex(p);
            row[x] = static_cast<int32_t>(glm::floor((v + 1.0f) * mHeightScalar / 2.0f));
            mMin = glm::min(row[x], mMin);
            mMax = glm::max(row[x], mMax);
        }
        mHeightMap[y] = std::move(row);
    }
}

std::optional<int32_t> HeightMap::at(const std::size_t u, const std::size_t v) const noexcept
{
    if (u >= mSize or v >= mSize)
    {
        return std::nullopt;
    }
    return mHeightMap[u][v];
}

std::optional<int32_t> HeightMap::at(const glm::ivec2& uv) const noexcept
{
    if (uv.x >= mSize or uv.y >= mSize)
    {
        return std::nullopt;
    }
    return mHeightMap[uv.x][uv.y];
}

glm::ivec2 HeightMap::randomPoint() const noexcept
{
    return Random::getVector<glm::ivec2>(0, static_cast<int32_t>(mSize));
}

int32_t HeightMap::sampleHeight() const noexcept
{
    const auto sample = randomPoint();
    return mHeightMap[sample.x][sample.y];
}

glm::vec3 HeightMap::sample() const noexcept
{
    const auto s = randomPoint();
    const auto y = mHeightMap[s.x][s.y];
    return { static_cast<float>(s.x), static_cast<float>(y), static_cast<float>(s.y) };
}

const std::vector<std::vector<int32_t>>& HeightMap::getHeightMap() const noexcept
{
    return mHeightMap;
}

int32_t HeightMap::getMin() const noexcept
{
    return mMin;
}

int32_t HeightMap::getMax() const noexcept
{
    return mMax;
}
