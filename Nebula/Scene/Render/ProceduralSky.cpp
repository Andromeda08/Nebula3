#include "ProceduralSky.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include "VulkanRHI/Barrier.hpp"

ProceduralSkyPass::ProceduralSkyPass(const ProceduralSkyPass_Params& params)
: RenderPass({{}, params.rhi, "ProceduralSky" })
, mParams(params.initialParams)
{
    createResources();
    createPipeline();


}

UPtr<ProceduralSkyPass> ProceduralSkyPass::create(const ProceduralSkyPass_Params& params) noexcept
{
    return makeUnique<ProceduralSkyPass>(params);
}

void ProceduralSkyPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    // if (!mParamsChanged)
    // {
    //     return;
    // }

    pCommandList->beginLabel("ProceduralSky_Pass");

    RHI::Barrier()
        .addBarrier(mCubeMap->getBarrier(RHI::ImageUsage::General))
        .addBarrier(mSkyData->getBarrier(RHI::BufferUsage::Fragment, RHI::BufferUsage::Compute))
        .insert(pCommandList);

    mPipeline->bind(pCommandList);
    mPipeline->bindDescriptorSets(pCommandList, { mDescriptor->getSet(0) });
    mPipeline->pushConstants(pCommandList, &mParams);
    mPipeline->dispatch(pCommandList, 128 / 8, 128 / 4, 6);

    mCubeMap->generateMipmaps(pCommandList, vk::Filter::eLinear);

    pCommandList->endLabel();

    mParamsChanged = false;
}

const SPtr<RHI::Image>& ProceduralSkyPass::getCubeMap() const noexcept
{
    return mCubeMap;
}

const SPtr<RHI::Buffer>& ProceduralSkyPass::getSkyDataBuffer() const noexcept
{
    return mSkyData;
}

void ProceduralSkyPass::setParams(const SkyParams& params) noexcept
{
    mParams = params;
    mParamsChanged = true;
}

void ProceduralSkyPass::createResources() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mCubeMap = mRHI->createImage({
        .extent        = { 128, 128 },
        .format        = vk::Format::eR16G16B16A16Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst | eStorage,
        .createSampler = true,
        .cubeMap       = true,
        .debugName     = "ProceduralSky_CubeMap",
    });

    mSkyData = mRHI->createBuffer({
        .size  = sizeof(glm::vec4) * 2,
        .type  = RHI::BufferType::Storage,
        .label = "ProceduralSky_Data",
    });

    mDescriptor = mRHI->createDescriptor({
       .bindings = {
           { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
           { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
       },
       .setCount = 1,
       .debugName = "ProceduralSky_Descriptor",
   });

    const auto descriptorWrite = RHI::DescriptorWrite()
        .writeStorageImage(0, vk::ImageLayout::eGeneral, mCubeMap)
        .writeStorageBuffer(1, mSkyData);
    mDescriptor->write(0, descriptorWrite);
}

void ProceduralSkyPass::createPipeline() noexcept
{
    auto pipelineCreateInfo = RHI::ComputePipelineCreateInfo()
        .addDescriptorSetLayout(mDescriptor->getLayout())
        .setPushConstantRange({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(SkyParams) })
        .setComputeShader(Configuration::getShaderFilePath("RayleighMieSky.comp.spv").string())
        .setDebugName("ProceduralSky_Pipeline");
    mPipeline = mRHI->createComputePipeline(pipelineCreateInfo);
}

void ProceduralSkyPassComponent::draw()
{
    ImGui::Begin("Sky Options");
    bool changed = ImGui::SliderFloat("Time of Day", &mTimeOfDay, 0.0f, 24.0f);
    changed |= ImGui::SliderFloat("Intensity", &mIntensity, 0.0f, 120.0f);

    if (changed)
    {
        mSkyPass->setParams(SkyParams::fromTimeOfDay(mTimeOfDay, mIntensity));
    }

    ImGui::End();
}
