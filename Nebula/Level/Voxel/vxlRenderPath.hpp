#pragma once

#include <VulkanRHI/VulkanRHI.hpp>

#include "Scene/Render/FXAAPass.hpp"
#include "Scene/Render/LightingPass.hpp"
#include "Scene/Render/SSAOPass.hpp"
#include "Scene/Render/Voxel_GBufferPass.hpp"

class VoxelScene;

class vxlRenderPath
{
public:
    explicit vxlRenderPath(const SPtr<RHI::VulkanRHI>& rhi, VoxelScene* pScene);

    void execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept;

    Size2D                  mRenderResolution;
    vk::Extent2D            mRenderExtent;

    UPtr<Voxel_GBufferPass> mGBuffer;
    UPtr<SSAOPass>          mSSAO;
    UPtr<LightingPass>      mLightingPass;
    UPtr<FXAAPass>          mFXAA;

    VoxelScene*             mScene;
    SPtr<RHI::VulkanRHI>    mRHI;

private:
    void execute_BlitToSwapchain(RHI::Image* pFinalImage, const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept;
};
