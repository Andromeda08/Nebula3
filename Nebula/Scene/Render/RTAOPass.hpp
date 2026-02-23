#pragma once

#include "RenderPass.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct RTAO_Input
{
    // View-space position in XYZ and linear fragment depth in W
    SPtr<RHI::Image> positionBuffer;
    // View-space normals
    SPtr<RHI::Image> normalBuffer;
    // Scene Descriptor (containing Camera UB, expected binding = 0)
    SPtr<RHI::Descriptor> sceneDescriptor;
};

struct RTAO_Params
{
    Size2D               resolution = {};
    RTAO_Input           input      = {};
    SPtr<RHI::VulkanRHI> rhi;
};

// Raytraced Ambient Occlusion + Denoising
class RTAOPass : public RenderPass
{
    struct PushConstants {
        float   aoRadius        = 1.0f;
        int32_t aoSamples       = 16;
        float   aoPower         = 2.0f;
        int32_t aoDistanceBased = 0;
        int32_t frameNumber     = 0;
    };

public:
    explicit RTAOPass(const RTAO_Params& params);

    [[nodiscard]] static UPtr<RTAOPass> create(const RTAO_Params& params) noexcept;

    ~RTAOPass() override = default;

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override;

    // Get the current RTAO result image
    [[nodiscard]] const SPtr<RHI::Image>& getResult() const noexcept;

private:
    void createResources_RTAO() noexcept;
    void createResources_Denoise() noexcept;

    void execute_RTAO(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;
    void execute_Denoise(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;

    static constexpr uint32_t sGroupSize = 16;

    RTAO_Input              mInput;

    // RTAO Pass
    // ============================
    SPtr<RHI::Image>            mRTAO_Result;

    PushConstants               mRTAO_PushConstants;
    SPtr<RHI::ComputePipeline>  mRTAO_Pipeline;
    SPtr<RHI::Descriptor>       mRTAO_Descriptor;

    // Denoise Pass
    // ============================
    SPtr<RHI::Image>            mDenoise_Result;

    SPtr<RHI::ComputePipeline>  mDenoise_Pipeline;
    SPtr<RHI::Descriptor>       mDenoise_Descriptor;
};
