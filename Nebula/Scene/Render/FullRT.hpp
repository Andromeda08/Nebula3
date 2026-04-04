#pragma once

#include "RenderPass.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct FullRT_Params
{
    SPtr<RHI::Descriptor>   sceneDescriptor;
    Size2D                  resolution;
    SPtr<RHI::VulkanRHI>    rhi;
};

class FullRTPass : public RenderPass
{
public:
    explicit FullRTPass(const FullRT_Params& params);

    [[nodiscard]] static UPtr<FullRTPass> create(const FullRT_Params& params) noexcept;

    ~FullRTPass() override = default;

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override;

    [[nodiscard]] SPtr<RHI::Image> getResult() const noexcept override;

private:
    void createResources() noexcept;
    void createPipeline()  noexcept;

    SPtr<RHI::Image>                mOutput;
    SPtr<RHI::Descriptor>           mSceneDescriptor;
    SPtr<RHI::Descriptor>           mDescriptor;
    SPtr<RHI::RaytracingPipeline>   mPipeline;
    SPtr<RHI::ShaderBindingTable>   mSBT;
};
