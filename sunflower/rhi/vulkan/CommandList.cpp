#include "CommandList.hpp"

namespace sunflower::rhi
{
    VulkanCommandList::VulkanCommandList(const VulkanCommandListCreateInfo& createInfo)
    : mHandle(createInfo.commandBuffer)
    {
    }

    void VulkanCommandList::begin()
    {
        mIsRecording = true;
        std::ignore = mHandle.begin(vk::CommandBufferBeginInfo());
    }

    void VulkanCommandList::end()
    {
        mIsRecording = false;
        std::ignore = mHandle.end();
    }
}
