#include "ComputePipeline.hpp"

namespace RHI
{
    ComputePipeline::ComputePipeline(ComputePipelineCreateInfo& createInfo)
    : Pipeline()
    {
        mPushConstantRange = createInfo.pushConstantRange;
        mDevice = createInfo.device;
        mPipelineType = PipelineType::Compute;
        mBindPoint = PipelineUtils::pipelineTypeToBindPoint(mPipelineType);

        const auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
            .setSetLayoutCount(createInfo.descriptorSetLayouts.size())
            .setPSetLayouts(createInfo.descriptorSetLayouts.data())
            .setPushConstantRangeCount(mPushConstantRange.size != 0 ? 1 : 0)
            .setPPushConstantRanges(mPushConstantRange.size != 0 ? &mPushConstantRange : nullptr);

        mPipelineLayout = mDevice->getHandle().createPipelineLayout(layoutCreateInfo);
        mDevice->nameObject<vk::PipelineLayout>({
            .debugName = std::format("{} Layout", createInfo.debugName),
            .handle    = mPipelineLayout,
        });

        const auto shader = Shader::compileShader(mDevice.get(), createInfo.computeShader);

        const auto computePipelineCreateInfo = vk::ComputePipelineCreateInfo()
            .setStage(shader.shaderStageInfo)
            .setLayout(mPipelineLayout);

        mPipeline = mDevice->getHandle().createComputePipeline(nullptr, computePipelineCreateInfo).value;
        mDevice->nameObject<vk::Pipeline>({
            .debugName = createInfo.debugName,
            .handle    = mPipeline,
        });
    }

    UPtr<ComputePipeline> ComputePipeline::create(ComputePipelineCreateInfo& createInfo)
    {
        return std::make_unique<ComputePipeline>(createInfo);
    }

    void ComputePipeline::dispatch(const vk::CommandBuffer& commandBuffer, const uint32_t sizeX, const uint32_t sizeY, const uint32_t sizeZ) const
    {
        commandBuffer.dispatch(sizeX, sizeY, sizeZ);
    }
}
