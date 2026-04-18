#pragma once

#include "RenderPass.hpp"
#include "Scene/TLASManager.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct Lighting_Input
{
    SPtr<RHI::Image>        position;
    SPtr<RHI::Image>        normal;
    SPtr<RHI::Image>        albedo;
    SPtr<RHI::Descriptor>   sceneDescriptor;
    SPtr<RHI::Image>        ssao;
    SPtr<RHI::Image>        lightingParams;
    TLASManager*            tlasManager;
    SPtr<RHI::Image>        cubeMap;
    SPtr<RHI::Buffer>       skyData;
};

struct Lighting_Params
{
    Size2D               resolution;
    Lighting_Input       input;
    SPtr<RHI::VulkanRHI> rhi;
};

// Deferred Lighting Pass
class LightingPass : public RenderPass
{
    struct PushConstants
    {
        int32_t shadowMode = 1;
    };
public:
    explicit LightingPass(const Lighting_Params& params);

    [[nodiscard]] static UPtr<LightingPass> create(const Lighting_Params& params) noexcept;

    ~LightingPass() override = default;

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override;

    [[nodiscard]] SPtr<RHI::Image> getResult() const noexcept override;

    void setShadowMode(int32_t mode) noexcept;

private:
    void createResources() noexcept;
    void createPipeline()  noexcept;

    PushConstants           mPushConstants;
    Lighting_Input          mInput;
    SPtr<RHI::Image>        mOutput;
    SPtr<RHI::Descriptor>   mDescriptor;
    SPtr<RHI::RenderPass>   mRenderPass;
    SPtr<RHI::Pipeline>     mPipeline;
};
