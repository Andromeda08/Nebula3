#pragma once

#include <vector>
#include "VoxelData.hpp"

namespace vxl
{
    class IFeatureGenerator
    {
    public:
        virtual ~IFeatureGenerator() = default;
        virtual std::vector<VoxelData> generate() noexcept = 0;
    };
}
