#pragma once

#include "RenderPass.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct FXAA_Input
{
    SPtr<RHI::Image> input;
};

struct FXAA_Params
{
    Size2D               resolution;
    FXAA_Input           input;
    SPtr<RHI::VulkanRHI> rhi;
};

// FXAA Anti-Aliasing Pass
class FXAAPass : public RenderPass
{
    struct PushConstant
    {
        float rcpX;
        float rcpY;
        float _pad0;
        float _pad1;
    };
public:
    explicit FXAAPass(const FXAA_Params& params);

    [[nodiscard]] static UPtr<FXAAPass> create(const FXAA_Params& params) noexcept;

    ~FXAAPass() override = default;

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override;

    [[nodiscard]] SPtr<RHI::Image> getResult() const noexcept override;

private:
    void createResources() noexcept;
    void createPipeline()  noexcept;

    FXAA_Input              mInput;
    SPtr<RHI::Image>        mOutput;
    SPtr<RHI::Descriptor>   mDescriptor;
    SPtr<RHI::RenderPass>   mRenderPass;
    SPtr<RHI::Pipeline>     mPipeline;
};
