#pragma once

#include <VulkanRHI/VulkanRHI.hpp>

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

    // Ambient Occlusion
    constexpr static uint32_t sSSAOKernelSize = 64;
    constexpr static uint32_t sSSAONoiseSize  = 8;
    constexpr static float    sSSAORadius     = 0.3f;

    vxlPass                 mSSAOPass;
    vxlPass                 mSSAO_BlurPass;
    SPtr<RHI::Image>        mSSAOBuffer;        // Attachment 0
    SPtr<RHI::Image>        mSSAO_BlurBuffer;   // Attachment 0
    SPtr<RHI::Image>        mSSAONoise;
    SPtr<RHI::Buffer>       mSSAOKernel;
    SPtr<RHI::Descriptor>   mSSAODescriptor;

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

    void resources_SSAOPass() noexcept;

    void create_SSAOPass() noexcept;

    void execute_SSAOPass(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept;

    void execute_BlitToSwapchain(RHI::Image* pFinalImage, const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept;
};
