#pragma once

#include "TonemapPass.hpp"
#include "Level/Render/BoundingBoxDebugPass.hpp"
#include "Level/Render/GBufferPass.hpp"
#include "Level/Render/LightingPass.hpp"
#include "Level/Render/TonemapPass.hpp"
#include "Scene/TextureManager.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class Level;

    class LevelRenderer
    {
    public:
        explicit LevelRenderer(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager, Level* pLevel);

        void render(const RHI::FrameData& frameData, const RHI::CommandList* commandList) const;

    private:
        void blitToSwapchain(RHI::Image* pImage, const RHI::CommandList* commandList, const RHI::FrameData& frameData) const;

        SPtr<RHI::VulkanRHI>        mRHI;
        TextureManager*             mTextureManager;
        Level*                      mLevel;

        SPtr<GBufferPass>           mGBufferPass;
        UPtr<LightingPass>          mLightingPass;
        UPtr<TonemapPass>           mTonemapPass;
        UPtr<BoundingBoxDebugPass>  mBoundingBoxDebugPass;
    };
}
