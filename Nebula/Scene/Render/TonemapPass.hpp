#pragma once

#include "RenderPass.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct Tonemap_Input
{
    SPtr<RHI::Image> color;
};

struct Tonemap_Params
{
    Size2D               resolution;
    SPtr<RHI::VulkanRHI> rhi;
    Tonemap_Input        input;
};

class TonemapPass : public RenderPass
{
public:
    explicit TonemapPass(const Tonemap_Params& params);

    [[nodiscard]] static UPtr<TonemapPass> create(const Tonemap_Params& params) noexcept;

    ~TonemapPass() override = default;

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override;

    [[nodiscard]] SPtr<RHI::Image> getResult() const noexcept override;

private:
    void createResources() noexcept;
    void createPipeline()  noexcept;

    Tonemap_Input           mInput;
    SPtr<RHI::Image>        mOutput;
    SPtr<RHI::Descriptor>   mDescriptor;
    SPtr<RHI::RenderPass>   mRenderPass;
    SPtr<RHI::Pipeline>     mPipeline;
};
