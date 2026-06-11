#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "IFeatureGenerator.hpp"
#include "VoxelData.hpp"
#include "Core/Types.hpp"
#include "Math/HeightMap.hpp"

namespace vxl
{
    struct TerrainGeneratorControl : public HeightMap::Control
    {
    };

    class TerrainGenerator
    {
    public:
        explicit TerrainGenerator(const TerrainGeneratorControl& control);

        template <class T, class... Args>
        requires std::is_base_of_v<IFeatureGenerator, T>
        void addGenerator(Args&&... args) noexcept
        {
            mGenerators.push_back(makeUnique<T>(mHeightMap, std::forward<Args>(args)...));
        }

        void generate() noexcept;

        [[nodiscard]] const std::vector<VoxelData>& getResult() const noexcept
        {
            return mVoxels;
        }

    private:
        [[nodiscard]] std::array<int32_t, 4> getNeighborhoodDeltas(const glm::ivec3& pos) const noexcept;

        [[nodiscard]] std::vector<VoxelData> fillGaps(const glm::ivec3& pos) const noexcept;

        constexpr static std::array<glm::ivec2, 4> sDirections = {
            glm::ivec2(0, -1), glm::ivec2(-1, 0), glm::ivec2(0, 1), glm::ivec2(0, 1)
        };

        TerrainGeneratorControl              mControl;
        glm::vec3                            mOffset;
        HeightMap                            mHeightMap;
        std::vector<UPtr<IFeatureGenerator>> mGenerators;
        std::vector<VoxelData>               mVoxels;
    };
}
