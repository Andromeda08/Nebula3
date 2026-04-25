#pragma once

#include "Level/Render/BoundingBoxDebugPass.hpp"
#include "Scene/TextureManager.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class Level;

    class LevelRenderer
    {
        struct PushConstants
        {
            uint64_t instanceBufferAddress;
            uint64_t instanceMapAddress;
            uint64_t cameraUniformAddress;
            uint64_t materialAddress;
        };
    public:
        explicit LevelRenderer(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager, Level* pLevel);

        void render(const RHI::FrameData& frameData, const RHI::CommandList* commandList) const;

    private:
        void blitToSwapchain(RHI::Image* pImage, const RHI::CommandList* commandList, const RHI::FrameData& frameData) const;

        SPtr<RHI::VulkanRHI>        mRHI;
        TextureManager*             mTextureManager;
        Level*                      mLevel;

        UPtr<BoundingBoxDebugPass>  mBoundingBoxDebugPass;

        SPtr<RHI::Image>            mAlbedoBuffer;
        SPtr<RHI::Image>            mDepthBuffer;

        vk::Rect2D                  mScissor;
        vk::Viewport                mViewport;
        SPtr<RHI::Pipeline>         mPipeline;
        SPtr<RHI::RenderPass>       mRenderPass;
    };
}
