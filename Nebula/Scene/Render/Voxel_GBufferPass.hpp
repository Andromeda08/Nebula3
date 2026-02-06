#pragma once

#include "RenderPass.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

class VoxelScene;

struct GBuffer_Params
{
    Size2D                  resolution;
    VoxelScene*             pScene;
    SPtr<RHI::VulkanRHI>    rhi;
};

// G-Buffer Pass for the Voxel scene
class Voxel_GBufferPass : public RenderPass
{
public:
    explicit Voxel_GBufferPass(const GBuffer_Params& params);

    [[nodiscard]] static UPtr<Voxel_GBufferPass> create(const GBuffer_Params& params) noexcept;

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override;

    [[nodiscard]] const SPtr<RHI::Image>& getPosition() const noexcept;

    [[nodiscard]] const SPtr<RHI::Image>& getNormal() const noexcept;

    [[nodiscard]] const SPtr<RHI::Image>& getAlbedo() const noexcept;

private:
    void createResources() noexcept;
    void createPipeline()  noexcept;

    VoxelScene*             mScene;

    SPtr<RHI::Image>        mPositionDepthBuffer;
    SPtr<RHI::Image>        mNormalBuffer;
    SPtr<RHI::Image>        mAlbedoBuffer;
    SPtr<RHI::Image>        mDepthBuffer;

    SPtr<RHI::Pipeline>     mPipeline;
    SPtr<RHI::RenderPass>   mRenderPass;
};
