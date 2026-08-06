#pragma once

#include "Shared.hpp"
#include "Hair/HairGeometry.hpp"
#include "Stages/Hybrid_MeshStage.hpp"
#include "Stages/Hybrid_SoftwareStage.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class HairRenderer
    {
    public:
        HairRenderer(const SPtr<RHI::VulkanRHI>& rhi, HairModelSystem* pHairModelSystem);

        ~HairRenderer();

        void execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const HairRenderer_BDAs& buffers);

        void drawUI();

        [[nodiscard]] const SPtr<RHI::Image>& getResult(uint32_t currentFrame);

        [[nodiscard]] const SPtr<RHI::Image>& getDepth(uint32_t currentFrame);

    private:
        friend class HairView;

        void execute_getQueryPoolResults(uint32_t currentFrame);
        void execute_beginQuery(const RHI::CommandList* pCommandList, uint32_t currentFrame) const;
        void execute_endQuery(const RHI::CommandList* pCommandList, uint32_t currentFrame);

        void readbackSmallTriangleCount(const RHI::CommandList* pCommandList, uint32_t currentFrame);

        void createStatsResources();

        const std::string               mLabel     = "HairRenderer";

        SPtr<RHI::VulkanRHI>            mRHI;
        HairModelSystem*                mHairModelSystem = nullptr;

        UPtr<HairShared>                mShared;

        UPtr<Hybrid_MeshStage>          mMeshStage;
        UPtr<Hybrid_SoftwareStage>      mSoftwareStage;

        // Statistics
        PerFrameArray<vk::QueryPool>     mMeshPrimitivePool;
        PerFrameArray<bool>              mMeshPrimitiveQueryValid {};
        PerFrameArray<SPtr<RHI::Buffer>> mReadBackTriCount;

        uint64_t                         mMeshTriangles  = 0;
        uint64_t                         mSmallTriangles = 0;
    };
}
