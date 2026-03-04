#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "RenderPass.hpp"
#include "UserInterface/IComponent.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct SkyParams
{
    glm::vec3 sunDirection;
    float     sunIntensity;

    [[nodiscard]] static SkyParams fromTimeOfDay(const float hours, const float intensity = 10.0f) noexcept
    {
        const float angle = (hours / 24.0f) * glm::two_pi<float>() - glm::half_pi<float>();
        return {
            .sunDirection = glm::normalize(glm::vec3(0.0f, glm::sin(angle), glm::cos(angle))),
            .sunIntensity = intensity,
        };
    }
};

struct ProceduralSkyPass_Params
{
    SkyParams            initialParams;
    SPtr<RHI::VulkanRHI> rhi;
};

class ProceduralSkyPass : public RenderPass
{
public:
    explicit ProceduralSkyPass(const ProceduralSkyPass_Params& params);

    [[nodiscard]] static UPtr<ProceduralSkyPass> create(const ProceduralSkyPass_Params& params) noexcept;

    ~ProceduralSkyPass() override = default;

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override;

    [[nodiscard]] const SPtr<RHI::Image>& getCubeMap() const noexcept;

    [[nodiscard]] const SPtr<RHI::Buffer>& getSkyDataBuffer() const noexcept;

    void setParams(const SkyParams& params) noexcept;

private:
    void createResources() noexcept;
    void createPipeline()  noexcept;

    SPtr<RHI::Image>            mCubeMap;
    SPtr<RHI::Buffer>           mSkyData;

    SkyParams                   mParams;
    bool                        mParamsChanged = true;

    SPtr<RHI::ComputePipeline>  mPipeline;
    SPtr<RHI::Descriptor>       mDescriptor;
};

class ProceduralSkyPassComponent : public IComponent
{
public:
    explicit ProceduralSkyPassComponent(ProceduralSkyPass* pSkyPass)
    : mSkyPass(pSkyPass)
    {
    }

    ~ProceduralSkyPassComponent() override = default;

    void draw() override;

private:
    float              mTimeOfDay = 16.0f;
    float              mIntensity = 10.0f;
    ProceduralSkyPass* mSkyPass;
};

