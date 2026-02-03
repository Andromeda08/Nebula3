#pragma once

#include <glm/glm.hpp>

#include "../IFeatureGenerator.hpp"
#include "Core/Random.hpp"
#include "Math/HeightMap.hpp"

namespace vxl
{
    class PillarGenerator final : public IFeatureGenerator
    {
    public:
        explicit PillarGenerator(HeightMap& heightMap, const size_t count)
        : IFeatureGenerator()
        , mCount(count)
        , mHeightMap(heightMap)
        {
        }

        ~PillarGenerator() override = default;

        std::vector<VoxelData> generate() noexcept override
        {
            static const std::vector<glm::vec3> colorPool = {
                {1.0f, 1.0f, 0.0f }, {0.0f, 0.8f, 1.0f },
                {1.0f, 0.0f, 0.25f}, {1.0f, 0.0f, 0.85f},
                {0.2f, 1.0f, 0.2f }, {0.1f, 0.8f, 0.9f },
                {1.0f, 0.1f, 0.1f }, {1.0f, 0.5f, 0.0f }
            };

            std::vector<VoxelData> result;

            for (size_t n = 0; n < mCount; n++)
            {
                const auto s = Random::get(2.0f, 4.0f);
                VoxelData voxel {
                    .position = mHeightMap.sample(),
                    .scale = glm::vec3(s, Random::get(4.0f, 16.0f), s),
                    .color = colorPool[Random::get(0, 12) % colorPool.size()],
                };
                result.push_back(voxel);
            }

            return result;
        }

    private:
        const size_t mCount;
        HeightMap& mHeightMap;
    };
}