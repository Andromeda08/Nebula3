#include "TerrainGenerator.hpp"

namespace vxl
{
    TerrainGenerator::TerrainGenerator(const TerrainGeneratorControl& control)
    : mControl(control)
    , mHeightMap(control)
    {
        const auto sizeOver2 = static_cast<float>(control.size) / 2.0f;
        mOffset = control.centered ? glm::vec3(sizeOver2, 0.0f, sizeOver2) : glm::vec3(0.0f);
    }

    void TerrainGenerator::generate() noexcept
    {
        auto pos = glm::ivec2(0);
        mVoxels.reserve(mControl.size * mControl.size);
        for (std::size_t y = 0; y <  mControl.size; y++)
        {
            const auto yPercent = static_cast<float>(y) / static_cast<float>(mControl.size);
            pos.y = static_cast<int32_t>(y);
            for (std::size_t x = 0; x < mControl.size; x++)
            {
                const auto xPercent = static_cast<float>(x) / static_cast<float>(mControl.size);

                pos.x = static_cast<int32_t>(x);
                if (const auto height = mHeightMap.at(pos); height.has_value())
                {
                    const auto value = height.value();
                    VoxelData voxel = {
                        .position = glm::vec3(x, value, y),
                        .scale    = glm::vec3(1.0f),
                        .color    = glm::vec3(xPercent, 0.0f, yPercent),
                    };
                    mVoxels.push_back(voxel);
                    mVoxels.append_range(fillGaps({ pos.x, value, pos.y }));
                }
            }
        }

        for (auto&& generator : mGenerators)
        {
            mVoxels.append_range(generator->generate());
        }

        for (auto& voxel : mVoxels)
        {
            voxel.position -= mOffset;
        }
    }

    std::array<int32_t, 4> TerrainGenerator::getNeighborhoodDeltas(const glm::ivec3& pos) const noexcept
    {
        std::array result = { 0, 0, 0, 0 };

        for (const auto [i, dir] : nbl::enumerate(sDirections))
        {
            if (const auto n = mHeightMap.at(glm::ivec2(pos.x, pos.z) + dir); n.has_value())
            {
                const auto nHeight = n.value();
                result[i] = nHeight - pos.y;
            }
        }

        return result;
    }

    std::vector<VoxelData> TerrainGenerator::fillGaps(const glm::ivec3& pos) const noexcept
    {
        std::vector<VoxelData> result;

        for (const auto deltas = getNeighborhoodDeltas(pos);
            const auto [i, d] : nbl::enumerate(deltas))
        {
            if (d > 1)
            {
                for (int32_t j = d - 1; j > 0; j--)
                {
                    VoxelData voxel = {
                        .position = glm::vec3(pos.x - sDirections[i].x, pos.y - j, pos.z - sDirections[i].y),
                        .color    = glm::vec3(1.0f),
                    };
                    result.push_back(voxel);
                }
            }
        }

        return result;
    }
}
