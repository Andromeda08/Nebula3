#include "CommandList.hpp"

namespace RHI
{
    CommandList::CommandList(const CommandListCreateInfo& createInfo)
    : mCommandBuffer(createInfo.commandBuffer)
    , mSingleTime(createInfo.singleTimeSubmit)
    , mDebug(Configuration::getConfig().rhi.debugFeatures)
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

    void CommandList::beginLabel(const std::array<float, 3>& color, const std::string& name) const
    {
        if (!mDebug)
        {
            return;
        }

        const auto label = vk::DebugUtilsLabelEXT()
            .setColor({ color[0], color[1], color[2], 1.0f })
            .setPLabelName(name.c_str());
        mCommandBuffer.beginDebugUtilsLabelEXT(label);
    }

    void CommandList::endLabel() const
    {
        if (!mDebug)
        {
            return;
        }

        mCommandBuffer.endDebugUtilsLabelEXT();
    }

}
