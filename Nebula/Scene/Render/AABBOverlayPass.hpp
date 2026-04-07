#pragma once

#include "RenderPass.hpp"
#include "Scene/TLASManager.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

class SceneV2;

struct AABBOverlay_Input
{
    SPtr<RHI::Image>    image;
    SceneV2*            pScene;
    SPtr<RHI::Image>    depthBuffer;
};

struct AABBOverlay_Params
{
    AABBOverlay_Input    input;
    Size2D               resolution;
    SPtr<RHI::VulkanRHI> rhi;
};

// Deferred Lighting Pass
class AABBOverlayPass : public RenderPass
{
    struct PushConstants
    {
        glm::vec4 min;
        glm::vec4 max;
    };
public:
    explicit AABBOverlayPass(const AABBOverlay_Params& params);

    [[nodiscard]] static UPtr<AABBOverlayPass> create(const AABBOverlay_Params& params) noexcept;

    ~AABBOverlayPass() override = default;

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override;

    [[nodiscard]] SPtr<RHI::Image> getResult() const noexcept override
    {
        return mInput.image;
    }

private:
    void createPipeline() noexcept;

    PushConstants           mPushConstants;
    AABBOverlay_Input       mInput;
    SPtr<RHI::RenderPass>   mRenderPass;
    SPtr<RHI::Pipeline>     mPipeline;
};
