#pragma once

#include <VulkanRHI/VulkanRHI.hpp>

#include "Scene/Render/SSAOPass.hpp"

class VoxelScene;

struct vxlPass
{
    SPtr<RHI::GraphicsPipeline> pipeline;
    SPtr<RHI::RenderPass>       renderPass;
};

class vxlRenderPath
{
public:
    explicit vxlRenderPath(const SPtr<RHI::VulkanRHI>& rhi, VoxelScene* pScene);

    void execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept;

    Size2D                  mRenderResolution;
    vk::Extent2D            mRenderExtent;

    // G-Buffer Pass
    vxlPass                 mGBufferPass;
    SPtr<RHI::Image>        mPositionBuffer;    // Attachment 0
    SPtr<RHI::Image>        mNormalBuffer;      // Attachment 1
    SPtr<RHI::Image>        mAlbedoBuffer;      // Attachment 2
    SPtr<RHI::Image>        mDepthImage;        // Depth Attachment

    UPtr<SSAOPass> mSSAO;

    // Lighting Pass
    vxlPass                 mLightingPass;
    SPtr<RHI::Image>        mLightingResult;    // Attachment 0

    // Anti-Aliasing
    vxlPass                 mFXAAPass;
    SPtr<RHI::Image>        mAABuffer;          // Attachment 0

    VoxelScene*             mScene;
    SPtr<RHI::VulkanRHI>    mRHI;

private:
    [[nodiscard]] vk::Rect2D getRenderArea() const noexcept;

    void resources_GBufferPass() noexcept;

    void create_GBufferPass() noexcept;

    void execute_GBufferPass(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept;

    void execute_BlitToSwapchain(RHI::Image* pFinalImage, const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept;
};
