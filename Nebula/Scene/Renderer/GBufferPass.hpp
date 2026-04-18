#pragma once

#include "Pass.hpp"

class SceneV2;

namespace nbl
{
    struct GBuffer_Input
    {
        SceneV2* pScene = nullptr;
    };

    struct GBuffer_Output
    {
        // XYZ - View Position | W - Linear Depth
        SPtr<RHI::Image> positionDepth;

        // XYZ - View Normal | W - Unused
        SPtr<RHI::Image> normal;

        // XYZ - Albedo | W - Alpha
        SPtr<RHI::Image> albedo;

        // XYZ - Emissive Color | W - Unused
        SPtr<RHI::Image> emissive;

        // Depth Buffer
        SPtr<RHI::Image> depth;
    };

    struct GBufferPassCreateInfo
    {
        GBuffer_Input    inputs = {};
        RenderPassParams params = {};
    };

    class GBufferPass : public RenderPass
    {
        struct PushConstants
        {
            uint64_t instanceBufferAddress;
            uint64_t instanceMapAddress;
        };
    public:
        nbl_DISABLE_COPY(GBufferPass);
        nbl_CTOR(GBufferPass);

        // Return the Albedo image as general render pass result.
        [[nodiscard]] const SPtr<RHI::Image>& getResult() const noexcept override
        {
            return mOutput.albedo;
        }

        // Return all GBuffer pass outputs.
        [[nodiscard]] const GBuffer_Output& getOutput() const noexcept
        {
            return mOutput;
        }

    protected:
        void renderPass(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override;

    private:
        SceneV2*                mScene;

        GBuffer_Input           mInput;
        GBuffer_Output          mOutput;

        SPtr<RHI::RenderPass>   mRenderPass;
        SPtr<RHI::Pipeline>     mPipeline;
    };
}
