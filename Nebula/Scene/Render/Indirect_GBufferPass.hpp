#pragma once

#include "RenderPass.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

class SceneV2;

struct Indirect_GBuffer_Params
{
    Size2D                  resolution;
    SceneV2*                pScene;
    SPtr<RHI::VulkanRHI>    rhi;
};

// G-Buffer Pass for SceneV2 (Indirect Draw and InstancePool)
class Indirect_GBufferPass : public RenderPass
{
    struct PushConstants
    {
        uint64_t instanceBufferAddress;
        uint64_t instanceMapAddress;
    };
public:
    explicit Indirect_GBufferPass(const Indirect_GBuffer_Params& params);

    [[nodiscard]] static UPtr<Indirect_GBufferPass> create(const Indirect_GBuffer_Params& params) noexcept;

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override;

    [[nodiscard]] SPtr<RHI::Image> getResult() const noexcept override;

    [[nodiscard]] const SPtr<RHI::Image>& getPosition() const noexcept;

    [[nodiscard]] const SPtr<RHI::Image>& getNormal() const noexcept;

    [[nodiscard]] const SPtr<RHI::Image>& getAlbedo() const noexcept;

    [[nodiscard]] const SPtr<RHI::Image>& getDepth() const noexcept;

private:
    void createResources() noexcept;
    void createPipeline()  noexcept;

    SceneV2*                mScene;

    SPtr<RHI::Image>        mPositionDepthBuffer;
    SPtr<RHI::Image>        mNormalBuffer;
    SPtr<RHI::Image>        mAlbedoBuffer;
    SPtr<RHI::Image>        mEmissiveBuffer;
    SPtr<RHI::Image>        mMotionVectors;
    SPtr<RHI::Image>        mDepthBuffer;

    SPtr<RHI::Pipeline>     mPipeline;
    SPtr<RHI::RenderPass>   mRenderPass;
};
