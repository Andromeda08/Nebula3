#pragma once

#include "RenderPass.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct SSAO_Input
{
    // View-space position in XYZ and linear fragment depth in W
    SPtr<RHI::Image> positionBuffer;
    // View-space normals
    SPtr<RHI::Image> normalBuffer;
    // Scene Descriptor (containing Camera UB, expected binding = 0)
    SPtr<RHI::Descriptor> sceneDescriptor;
};

struct SSAO_Params
{
    bool                 useBlur    = true;
    Size2D               resolution = {};
    SSAO_Input           input      = {};
    SPtr<RHI::VulkanRHI> rhi;
};

// Screen-Space Ambient Occlusion
class SSAOPass : public RenderPass
{
public:
    explicit SSAOPass(const SSAO_Params& params);

    [[nodiscard]] static UPtr<SSAOPass> create(const SSAO_Params& params) noexcept;

    ~SSAOPass() override = default;

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override;

    // Get the current SSAO result image (depending on mRunBlurPass)
    [[nodiscard]] const SPtr<RHI::Image>& getResult() const noexcept;

    // Get the result Image of the SSAO RenderPass
    [[nodiscard]] const SPtr<RHI::Image>& getSSAOResult() const noexcept;

    // Get the result Image of the SSAO Blur RenderPass
    [[nodiscard]] const SPtr<RHI::Image>& getBlurredResult() const noexcept;

private:
    void createKernel() noexcept;
    void createNoiseTexture() noexcept;

    void createResources_SSAO() noexcept;
    void createResources_Blur() noexcept;

    void execute_SSAO(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;
    void execute_Blur(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;

    constexpr static uint32_t sKernelSize = 64;
    constexpr static uint32_t sNoiseSize  = 8;
    constexpr static float    sRadius     = 0.5f;

    SSAO_Input              mInput;
    bool                    mRunBlurPass;

    // SSAO Pass
    // ============================
    SPtr<RHI::Image>        mSSAO_Result;
    SPtr<RHI::Image>        mSSAO_Noise;
    SPtr<RHI::Buffer>       mSSAO_Kernel;

    SPtr<RHI::Pipeline>     mSSAO_Pipeline;
    SPtr<RHI::RenderPass>   mSSAO_RenderPass;
    SPtr<RHI::Descriptor>   mSSAO_Descriptor;

    // SSAO Blur Pass
    // ============================
    SPtr<RHI::Image>        mBlur_Result;

    SPtr<RHI::Pipeline>     mBlur_Pipeline;
    SPtr<RHI::RenderPass>   mBlur_RenderPass;
    SPtr<RHI::Descriptor>   mBlur_Descriptor;
};
