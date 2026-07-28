#pragma once

#include "SSAOPass.hpp"
#include "Level/Render/AntiAliasingPass.hpp"
#include "Level/Render/BoundingBoxDebugPass.hpp"
#include "Level/Render/GBufferPass.hpp"
#include "Level/Render/LightingPass.hpp"
#include "Level/Render/TonemapPass.hpp"
#include "VulkanRHI/VulkanRHI.hpp"
#include "../TextureManager.hpp"

namespace nbl
{
    class Level;

    class LevelRenderer
    {
    public:
        explicit LevelRenderer(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager, Level* pLevel);

        void render(const RHI::FrameData& frameData, RHI::CommandList* commandList) const;

    private:
        SPtr<RHI::VulkanRHI>        mRHI;
        TextureManager*             mTextureManager;
        Level*                      mLevel;

        UPtr<PrePass>               mPrePass;
        SPtr<GBufferPass>           mGBufferPass;
        UPtr<SSAOPass>              mSSAOPass;
        UPtr<LightingPass>          mLightingPass;
        UPtr<TonemapPass>           mTonemapPass;
        UPtr<AntiAliasingPass>      mAntiAliasingPass;
        UPtr<BoundingBoxDebugPass>  mBoundingBoxDebugPass;
    };
}
