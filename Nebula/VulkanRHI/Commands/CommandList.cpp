#include "CommandList.hpp"

#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/Image.hpp"

namespace RHI
{
    CommandList::CommandList(const CommandListCreateInfo& createInfo)
    : mCommandBuffer(createInfo.commandBuffer)
    , mSingleTime(createInfo.singleTimeSubmit)
    , mDebug(Configuration::getConfig().enableDebugFeatures)
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

    void CommandList::beginLabel(const std::string& name) const
    {
        if (!mDebug)
        {
            return;
        }

        const auto label = vk::DebugUtilsLabelEXT()
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

    void CommandList::copyBufferToImage(const BufferImageCopyInfo& copyInfo) const
    {
        const auto imageProperties = copyInfo.pDstImage->getProperties();
        const auto copyRegion = vk::BufferImageCopy2()
            .setBufferOffset(copyInfo.bufferOffset)
            .setBufferRowLength(0)
            .setBufferImageHeight(0)
            .setImageSubresource({ copyInfo.pDstImage->getProperties().aspectFlags, 0, 0, 1})
            .setImageOffset({0, 0, 0})
            .setImageExtent(imageProperties.getExtent3D());
        const auto copyBufferToImageInfo = vk::CopyBufferToImageInfo2()
            .setDstImage(copyInfo.pDstImage->getImage())
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcBuffer(copyInfo.pSrcBuffer->getHandle())
            .setRegions(copyRegion);

        mCommandBuffer.copyBufferToImage2(copyBufferToImageInfo);
    }

    void CommandList::copyBuffer(const BufferCopyInfo& bufferCopyInfo) const
    {
        if (!bufferCopyInfo.src)
        {
            spdlog::error("copyBuffer src is null");
            return;
        }
        if (!bufferCopyInfo.dst)
        {
            spdlog::error("copyBuffer dst is null");
            return;
        }

        std::vector<vk::BufferCopy2> regions;
        if (bufferCopyInfo.regions.empty())
        {
            regions.push_back({ 0, 0, bufferCopyInfo.src->getSize() });
        }
        else
        {
            for (const auto& region : bufferCopyInfo.regions)
            {
                regions.push_back({ region.srcOffset, region.dstOffset, region.size });
            }
        }

        const auto copyInfo = vk::CopyBufferInfo2()
            .setSrcBuffer(bufferCopyInfo.src->getHandle())
            .setDstBuffer(bufferCopyInfo.dst->getHandle())
            .setRegions(regions);

        mCommandBuffer.copyBuffer2(copyInfo);
    }
}
