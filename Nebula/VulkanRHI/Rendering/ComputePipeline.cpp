#include "ComputePipeline.hpp"

namespace RHI
{
    ComputePipeline::ComputePipeline(ComputePipelineCreateInfo& createInfo)
    : Pipeline()
    {
        mPushConstantRange = createInfo.pushConstantRange;
        mDevice = createInfo.device;
        mPipelineType = PipelineType::Graphics;
        mBindPoint = PipelineUtils::pipelineTypeToBindPoint(mPipelineType);

        assert(createInfo.shaderInfos.contains(vk::ShaderStageFlagBits::eCompute));

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

        const auto shaders = Shader::compileShaders(mDevice.get(), createInfo.shaderInfos);
        const auto shaderStageInfos = getStageCreateInfos(shaders);

        const auto computePipelineCreateInfo = vk::ComputePipelineCreateInfo()
            .setStage(shaderStageInfos[0])
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
}
