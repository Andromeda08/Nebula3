#pragma once

#include <set>
#include "Level/Geometry/GeometrySystem.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    struct PendingASRelease
    {
        SPtr<RHI::AccelerationStructure> buffer;
        uint64_t                         frameToRelease;
    };

    class BLASSystem
    {
    public:
        explicit BLASSystem(const SPtr<RHI::VulkanRHI>& rhi, GeometrySystem* pGeometrySystem);

        void onUpdate(const RHI::FrameData& frameData, const RHI::CommandList* pCommandList);

        [[nodiscard]] uint64_t getGeometryBlasAddress(int32_t index) const noexcept;

    private:
        [[nodiscard]] std::vector<int32_t> collectBuildGeometryIndices() const;

        SPtr<RHI::VulkanRHI>                            mRHI;
        GeometrySystem*                                 mGeometrySystem;

        // Track which geometries had their BLAS built already.
        std::set<int32_t>                               mHasBlas;

        SPtr<RHI::Buffer>                               mStaging;
        std::vector<PendingRelease>                     mPendingReleases;
        std::vector<PendingASRelease>                   mPendingASRelease;

        // Index directly maps to GeometryIndex
        std::vector<SPtr<RHI::AccelerationStructure>>   mBottomLevel;
        SPtr<RHI::Buffer>                               mBottomLevelData;

        static constexpr uint64_t sBlasAlignment = 256;

        [[nodiscard]] static constexpr uint64_t alignBLAS(const uint64_t x)
        {
            return (x + sBlasAlignment - 1) & ~(sBlasAlignment - 1);
        }
    };
}
