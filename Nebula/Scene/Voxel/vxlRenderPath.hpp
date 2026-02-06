#pragma once

#include <VulkanRHI/VulkanRHI.hpp>

#include "Scene/Render/SSAOPass.hpp"
#include "Scene/Render/Voxel_GBufferPass.hpp"

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

    UPtr<Voxel_GBufferPass> mGBuffer;
    UPtr<SSAOPass>          mSSAO;

    VoxelScene*             mScene;
    SPtr<RHI::VulkanRHI>    mRHI;

private:
    void execute_BlitToSwapchain(RHI::Image* pFinalImage, const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept;
};
