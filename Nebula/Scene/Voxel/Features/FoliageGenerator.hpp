#pragma once

#include <random>
#include <vector>
#include <glm/glm.hpp>

#include "../IFeatureGenerator.hpp"
#include "Core/Random.hpp"
#include "Math/HeightMap.hpp"

namespace vxl
{
    class FoliageGenerator final : public IFeatureGenerator
    {
        struct Foliage
        {
            VoxelData voxel;
            int32_t   patchIndex;
        };
    public:
        struct Control
        {
            size_t   patchCount;
            uint32_t patchRadius;
            uint32_t radiusVariance;
            float    density;
            bool     patchDensityVariance = true;
            bool     instanceRandomOffset = true;
            bool     instanceRandomScale  = true;
        };

        FoliageGenerator(HeightMap& heightMap, const Control& control);

        [[nodiscard]] std::vector<VoxelData> generate() noexcept override;

    private:
        [[nodiscard]] std::vector<Foliage> generateFoliagePatches() const noexcept;

        [[nodiscard]] std::vector<Foliage> generateFoliagePatch(const glm::ivec2& center, uint32_t r, uint32_t patchIndex) const noexcept;

        void placeFoliagePiece(std::vector<Foliage>& result, const glm::ivec2& pos, uint32_t patchIndex, float patchDensity) const noexcept;

        int32_t    mRadiusVariance;
        Control    mControl;
        HeightMap& mHeightMap;
    };
}