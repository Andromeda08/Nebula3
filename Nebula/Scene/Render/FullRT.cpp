#include "FullRT.hpp"

#include "VulkanRHI/Barrier.hpp"

FullRTPass::FullRTPass(const FullRT_Params& params)
: RenderPass({ params.resolution, params.rhi, "RayTracing" })
, mSceneDescriptor(params.sceneDescriptor)
{
    createResources();
    createPipeline();
}

UPtr<FullRTPass> FullRTPass::create(const FullRT_Params& params) noexcept
{
    return makeUnique<FullRTPass>(params);
}

void FullRTPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    pCommandList->beginLabel("FullRT_Pass");

    RHI::Barrier()
        .addBarrier(mOutput->getBarrier(RHI::ImageUsage::StorageImage))
        .insert(pCommandList);

    mPipeline->bind(pCommandList);
    mPipeline->bindDescriptorSets(pCommandList, { mSceneDescriptor->getSet(frameData.currentFrame), mDescriptor->getSet(0) }, 0);
    mPipeline->traceRays(pCommandList->getHandle(), mRenderResolution.width, mRenderResolution.height);

    pCommandList->endLabel();
}

SPtr<RHI::Image> FullRTPass::getResult() const noexcept
{
    return mOutput;
}

void FullRTPass::createResources() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mOutput = mRHI->createImage({
        .extent        = mRenderResolution,
        .format        = vk::Format::eR32G32B32A32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst | eStorage,
        .debugName     = "RT_Output",
    });

    mDescriptor = mRHI->createDescriptor({
       .bindings = {
           { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR },
       },
       .setCount = 1,
       .debugName = "RT_Descriptor",
   });

    const auto descriptorWrite = RHI::DescriptorWrite()
        .writeStorageImage(0, vk::ImageLayout::eGeneral, mOutput);
    mDescriptor->write(0, descriptorWrite);
}

void FullRTPass::createPipeline() noexcept
{
    const auto pipelineCreateInfo = RHI::RaytracingPipelineCreateInfo()
        .addDescriptorSetLayout(mSceneDescriptor->getLayout())
        .addDescriptorSetLayout(mDescriptor->getLayout())
        .addShader({ Configuration::getShaderFilePath("rt.rgen.spv"), vk::ShaderStageFlagBits::eRaygenKHR, "main" })
        .addShader({ Configuration::getShaderFilePath("rt.rmiss.spv"), vk::ShaderStageFlagBits::eMissKHR, "main" })
        .addShader({ Configuration::getShaderFilePath("rt.rchit.spv"), vk::ShaderStageFlagBits::eClosestHitKHR, "main" })
        .setRayDepth(1)
        .setDebugName("RT_Pipeline");

    mPipeline = mRHI->createRaytracingPipeline(pipelineCreateInfo);
}
