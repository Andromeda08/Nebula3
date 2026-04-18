#pragma once

#include "RenderPass.hpp"
#include "Scene/TLASManager.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct Bloom_Input
{
    SPtr<RHI::Image> emissive;
    SPtr<RHI::Image> lighting;
};

struct Bloom_Params
{
    Size2D               resolution;
    Bloom_Input          input;
    SPtr<RHI::VulkanRHI> rhi;
};

// Deferred Lighting Pass
class BloomPass : public RenderPass
{
    struct PushConstants
    {
        glm::vec2 direction;
    };
public:
    explicit BloomPass(const Bloom_Params& params);

    [[nodiscard]] static UPtr<BloomPass> create(const Bloom_Params& params) noexcept
    {
        return makeUnique<BloomPass>(params);
    }

    ~BloomPass() override = default;

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override;

    [[nodiscard]] SPtr<RHI::Image> getResult() const noexcept override;

private:
    void createResources() noexcept;
    void createPipeline()  noexcept;

    Bloom_Input             mInput;
    SPtr<RHI::Image>        mBloomHorizontal;
    SPtr<RHI::Image>        mBloomVertical;
    SPtr<RHI::Image>        mOutput;

    SPtr<RHI::Descriptor>   mBloomDescriptorH;
    SPtr<RHI::Descriptor>   mBloomDescriptorV;
    SPtr<RHI::RenderPass>   mBloomRenderPass;
    SPtr<RHI::Pipeline>     mBloomPipeline;

    SPtr<RHI::Descriptor>   mCompositeDescriptor;
    SPtr<RHI::RenderPass>   mCompositeRenderPass;
    SPtr<RHI::Pipeline>     mCompositePipeline;
};
