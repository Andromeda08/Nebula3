#include "Pipeline.hpp"

namespace RHI
{
    Pipeline::~Pipeline()
    {
        mDevice->getHandle().destroyPipeline(mPipeline);
        mDevice->getHandle().destroyPipelineLayout(mPipelineLayout);
    }

    void Pipeline::bind(const vk::CommandBuffer& commandBuffer) const
    {
        commandBuffer.bindPipeline(mBindPoint, mPipeline);
    }

    void Pipeline::bindDescriptorSet(const vk::CommandBuffer& commandBuffer, const vk::DescriptorSet& descriptorSet) const
    {
        commandBuffer.bindDescriptorSets(mBindPoint, mPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    }

    void Pipeline::bindDescriptorSets(const vk::CommandBuffer& commandBuffer, const std::vector<vk::DescriptorSet>& descriptorSets) const
    {
        commandBuffer.bindDescriptorSets(mBindPoint, mPipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
    }

    void Pipeline::pushConstants(const vk::CommandBuffer& commandBuffer, const void* pData) const
    {
        commandBuffer.pushConstants(mPipelineLayout, mPushConstantRange.stageFlags, mPushConstantRange.offset, mPushConstantRange.size, pData);
    }
}