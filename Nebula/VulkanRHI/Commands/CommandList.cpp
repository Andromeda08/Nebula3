#include "CommandList.hpp"

namespace RHI
{
    CommandList::CommandList(const CommandListCreateInfo& createInfo)
    : mCommandBuffer(createInfo.commandBuffer)
    , mSingleTime(createInfo.singleTimeSubmit)
    {
    }

    void CommandList::begin()
    {
        assert(!mIsRecording);
        mIsRecording = true;

        auto beginInfo = vk::CommandBufferBeginInfo();
        if (mSingleTime)
        {
            beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        }

        mCommandBuffer.begin(beginInfo);
    }

    void CommandList::end()
    {
        assert(mIsRecording);
        mIsRecording = false;

        mCommandBuffer.end();
    }
}
