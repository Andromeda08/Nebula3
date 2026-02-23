#include "RTAOPass.hpp"

#include "VulkanRHI/Barrier.hpp"

RTAOPass::RTAOPass(const RTAO_Params& params)
: RenderPass({ params.resolution, params.rhi, "RTAO" })
, mInput(params.input)
{
    createResources_RTAO();
    createResources_Denoise();
}

UPtr<RTAOPass> RTAOPass::create(const RTAO_Params& params) noexcept
{
    return makeUnique<RTAOPass>(params);
}

void RTAOPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    mRTAO_PushConstants.frameNumber++;
    execute_RTAO(pCommandList, frameData);
}

const SPtr<RHI::Image>& RTAOPass::getResult() const noexcept
{
    return mRTAO_Result;
}

void RTAOPass::createResources_RTAO() noexcept
{
    using enum vk::ImageUsageFlagBits;
    mRTAO_Result = mRHI->createImage({
        .extent        = { mRenderResolution.width, mRenderResolution.height },
        .format        = vk::Format::eR32Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst | eStorage,
        .createSampler = true,
        .debugName     = "RTAO_Result",
    });

     mRTAO_Descriptor = mRHI->createDescriptor({
        .bindings = {
            { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
            { 1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
            { 2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
        },
        .setCount = 1,
        .debugName = "RTAO_Descriptor",
    });

    const auto descriptorWrite = RHI::DescriptorWrite()
        .writeStorageImage(0, vk::ImageLayout::eGeneral, mInput.positionBuffer)
        .writeStorageImage(1, vk::ImageLayout::eGeneral, mInput.normalBuffer)
        .writeStorageImage(2, vk::ImageLayout::eGeneral, mRTAO_Result);
    mRTAO_Descriptor->write(0, descriptorWrite);

    auto pipelineCreateInfo = RHI::ComputePipelineCreateInfo()
        .addDescriptorSetLayout(mRTAO_Descriptor->getLayout())
        .addDescriptorSetLayout(mInput.sceneDescriptor->getLayout())
        .setPushConstantRange({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushConstants) })
        .setComputeShader(Configuration::getShaderFilePath("RTAO.comp.spv"))
        .setDebugName("RTAO_Pipeline");
    mRTAO_Pipeline = mRHI->createComputePipeline(pipelineCreateInfo);
}

void RTAOPass::createResources_Denoise() noexcept
{
}

void RTAOPass::execute_RTAO(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
{
    const auto [w, h] = mRHI->getSwapchain()->getProperties().extent;
    const auto groupX = (w + (sGroupSize - 1)) / sGroupSize;
    const auto groupY = (h + (sGroupSize - 1)) / sGroupSize;

    pCommandList->beginLabel("RTAO_Main_Pass");
    RHI::Barrier()
        .addBarrier(mInput.positionBuffer->getBarrier(RHI::ImageUsage::StorageImage))
        .addBarrier(mInput.normalBuffer->getBarrier(RHI::ImageUsage::StorageImage))
        .addBarrier(mRTAO_Result->getBarrier(RHI::ImageUsage::StorageImage))
        .insert(pCommandList);

    mRTAO_Pipeline->bind(pCommandList);
    mRTAO_Pipeline->bindDescriptorSets(pCommandList, { mRTAO_Descriptor->getSet(0), mInput.sceneDescriptor->getSet(frameData.currentFrame) });
    mRTAO_Pipeline->pushConstants(pCommandList, &mRTAO_PushConstants);
    mRTAO_Pipeline->dispatch(pCommandList, groupX, groupY);

    pCommandList->endLabel();
}

void RTAOPass::execute_Denoise(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
{
}
