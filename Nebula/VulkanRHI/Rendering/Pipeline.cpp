#include "Pipeline.hpp"

namespace RHI
{
    Pipeline::~Pipeline()
    {
        mDevice->waitIdle();
        mDevice->getHandle().destroyPipeline(mPipeline);
        mDevice->getHandle().destroyPipelineLayout(mPipelineLayout);
    }

    void Pipeline::bind(const vk::CommandBuffer& commandBuffer) const
    {
        commandBuffer.bindPipeline(mBindPoint, mPipeline);
    }

    void Pipeline::bind(const CommandList* pCommandList) const
    {
        pCommandList->getHandle().bindPipeline(mBindPoint, mPipeline);
    }

    void Pipeline::bindDescriptorSet(const vk::CommandBuffer& commandBuffer, const vk::DescriptorSet& descriptorSet) const
    {
        commandBuffer.bindDescriptorSets(mBindPoint, mPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    }

    void Pipeline::bindDescriptorSet(const CommandList* pCommandList, const vk::DescriptorSet& descriptorSet, const uint32_t setIndex) const
    {
        pCommandList->getHandle().bindDescriptorSets(mBindPoint, mPipelineLayout, setIndex, 1, &descriptorSet, 0, nullptr);
    }

    void Pipeline::bindDescriptorSets(const vk::CommandBuffer& commandBuffer, const std::vector<vk::DescriptorSet>& descriptorSets) const
    {
        commandBuffer.bindDescriptorSets(mBindPoint, mPipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
    }

    void Pipeline::bindDescriptorSets(const CommandList* pCommandList, const std::vector<vk::DescriptorSet>& descriptorSets, const uint32_t firstSet) const
    {
        pCommandList->getHandle().bindDescriptorSets(mBindPoint, mPipelineLayout, firstSet, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
    }

    void Pipeline::pushConstants(const vk::CommandBuffer& commandBuffer, const void* pData) const
    {
        commandBuffer.pushConstants(mPipelineLayout, mPushConstantRange.stageFlags, mPushConstantRange.offset, mPushConstantRange.size, pData);
    }

    void Pipeline::pushConstants(const CommandList* pCommandList, const void* pData) const
    {
        pCommandList->getHandle().pushConstants(mPipelineLayout, mPushConstantRange.stageFlags, mPushConstantRange.offset, mPushConstantRange.size, pData);
    }

    const vk::Pipeline& Pipeline::getHandle() const noexcept
    {
        return mPipeline;
    }

    const vk::PipelineLayout& Pipeline::getPipelineLayout() const noexcept
    {
        return mPipelineLayout;
    }
}
