#include "LevelRenderer.hpp"

#include "Templates.hpp"
#include "Level/Level.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace nbl
{
    LevelRenderer::LevelRenderer(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager, Level* pLevel)
    : mRHI(rhi)
    , mTextureManager(pTextureManager)
    , mLevel(pLevel)
    {
        mPrePass = PrePass::create({
            .pLevel = mLevel,
            .rhi = mRHI,
        });

        mGBufferPass = GBufferPass::create({
            .pTextureManager = mTextureManager,
            .pLevel          = mLevel,
            .rhi             = mRHI,
        });

        mLightingPass = LightingPass::create({
            .pGBufferPass    = mGBufferPass,
            .pTextureManager = mTextureManager,
            .pLevel          = mLevel,
            .rhi             = mRHI,
        });

        mTonemapPass = TonemapPass::create({
            .outputFormat = mLightingPass->getResult(0)->getProperties().format,
            .rhi          = mRHI,
        });

        mAntiAliasingPass = AntiAliasingPass::create({
            .input = mTonemapPass->getResults(),
            .rhi   = mRHI,
        });

        mBoundingBoxDebugPass = BoundingBoxDebugPass::create({
            .pLevel             = mLevel,
            .renderTargets       = mTonemapPass->getResults(),
            .gBufferDepthBuffer = mGBufferPass->getDepthBuffer(),
            .rhi                = mRHI,
        });
    }

    void LevelRenderer::render(const RHI::FrameData& frameData, RHI::CommandList* commandList) const
    {
        mPrePass->execute(commandList, frameData);
        mGBufferPass->execute(commandList, frameData, mPrePass->getDepthBuffer(frameData.currentFrame));
        mLightingPass->execute(commandList, frameData);
        mTonemapPass->execute(mLightingPass->getResult(frameData.currentFrame), commandList, frameData);
        mAntiAliasingPass->execute(commandList, frameData);
        mBoundingBoxDebugPass->execute(commandList, frameData);

        commandList->blitToSwapchain(mAntiAliasingPass->getResult(frameData.currentFrame).get(), mRHI->getSwapchain(), frameData.acquiredIndex);
    }
}
