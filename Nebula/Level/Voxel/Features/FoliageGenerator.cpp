#include "FoliageGenerator.hpp"

namespace vxl
{
    FoliageGenerator::FoliageGenerator(HeightMap& heightMap, const Control& control)
    : IFeatureGenerator()
    , mControl(control)
    , mHeightMap(heightMap)
    {
        mRadiusVariance = static_cast<int32_t>(control.radiusVariance);
    }

    std::vector<VoxelData> FoliageGenerator::generate() noexcept
    {
        auto foliage = generateFoliagePatches();

        std::vector<glm::vec3> foliageGroupColors;
        for (int32_t i = 0; i < mControl.patchCount; i++)
        {
            foliageGroupColors.emplace_back(Random::unit(), Random::unit(), Random::unit());
        }

        std::vector<VoxelData> result;

        for (auto&& f : foliage)
        {
            if (f.patchIndex != -1)
            {
                f.voxel.color = foliageGroupColors[f.patchIndex];
            }
            result.push_back(f.voxel);
        }

        return result;
    }

    std::vector<FoliageGenerator::Foliage> FoliageGenerator::generateFoliagePatches() const noexcept
    {
        std::vector<Foliage> result;
        for (size_t i = 0; i < mControl.patchCount; i++)
        {
            const auto center = mHeightMap.randomPoint();
            const auto r      = mControl.patchRadius + Random::get(-mRadiusVariance, mRadiusVariance);
            result.append_range(generateFoliagePatch(center, r, i));
        }
        return result;
    }

    std::vector<FoliageGenerator::Foliage> FoliageGenerator::generateFoliagePatch(const glm::ivec2& center,
        const uint32_t r, const uint32_t patchIndex) const noexcept
    {
        std::vector<Foliage> result;

        const auto patchDensity = mControl.patchDensityVariance
                                  ? mControl.density - (1.0f - mControl.density) * Random::unit()
                                  : mControl.density;

        int32_t x = 0;
        auto    y = static_cast<int32_t>(r);
        int32_t d = 1 - static_cast<int32_t>(r);

        while (x <= y)
        {
            for (int32_t i = center.x - x; i <= center.x + x; i++)
            {
                placeFoliagePiece(result, glm::ivec2(i, center.y + y), patchIndex, patchDensity);
                placeFoliagePiece(result, glm::ivec2(i, center.y - y), patchIndex, patchDensity);
            }
            if (x != y)
            {
                for (int32_t i = center.x - y; i <= center.x + y; i++)
                {
                    placeFoliagePiece(result, glm::ivec2(i, center.y + x), patchIndex, patchDensity);
                    placeFoliagePiece(result, glm::ivec2(i, center.y - x), patchIndex, patchDensity);
                }
            }

            x++;
            if (d < 0)
            {
                d += 2 * x + 3;
            }
            else
            {
                y--;
                d += 2 * (x - y) + 5;
            }
        }

        return result;
    }

    void FoliageGenerator::placeFoliagePiece(std::vector<Foliage>& result, const glm::ivec2& pos,
        const uint32_t patchIndex, const float patchDensity) const noexcept
    {
        if (const auto height = mHeightMap.at(pos); height.has_value())
        {
            if (Random::unit() < glm::clamp(patchDensity, 0.0f, 1.0f))
            {
                auto position = glm::vec3(static_cast<float>(pos.x), static_cast<float>(height.value()) + 1.0f, static_cast<float>(pos.y));

                if (mControl.instanceRandomOffset)
                {
                    position.x += (Random::unit() - 1.0f) / 2.0f;
                    position.z += (Random::unit() - 1.0f) / 2.0f;
                }

                auto scale = 1.0f;
                if (mControl.instanceRandomScale)
                {
                    scale      = Random::unit();
                    position.y -= (1.0f - scale) / 2.0f;
                }

                Foliage foliage {};
                foliage.voxel.position = position;
                foliage.voxel.scale    = glm::vec3(scale);
                foliage.patchIndex     = patchIndex;
                result.push_back(foliage);
            }
        }
    }
}
