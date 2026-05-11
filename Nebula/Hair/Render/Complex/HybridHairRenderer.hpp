#pragma once

#include "HybridHair_MeshStage.hpp"
#include "HybridHair_SoftwareStage.hpp"
#include "Shared.hpp"
#include "Hair/HairGeometry.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class HybridHairRenderer
    {
    public:
        HybridHairRenderer(const SPtr<RHI::VulkanRHI>& rhi, HairModelSystem* pHairModelSystem)
        : mRHI(rhi)
        , mHairModelSystem(pHairModelSystem)
        {
            mShared        = makeUnique<HairShared>(rhi.get(), mHairModelSystem);
            mMeshStage     = makeUnique<HybridHair_MeshStage>(mRHI, mShared.get());
            mSoftwareStage = makeUnique<HybridHair_SoftwareStage>(mRHI, mShared.get());
        }

        void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const uint64_t cameraBufferAddress) const
        {
            pCommandList->beginLabel("HybridHairRenderer");

            mMeshStage->execute(pCommandList, frameData, cameraBufferAddress);
            mSoftwareStage->execute(pCommandList, frameData);

            pCommandList->endLabel();
        }

    private:
        SPtr<RHI::VulkanRHI>            mRHI;
        HairModelSystem*                mHairModelSystem;

        UPtr<HairShared>                mShared;

        UPtr<HybridHair_MeshStage>      mMeshStage;
        UPtr<HybridHair_SoftwareStage>  mSoftwareStage;
    };
}
