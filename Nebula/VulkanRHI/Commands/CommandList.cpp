#include "CommandList.hpp"

#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/Rendering/Pipeline.hpp"

namespace RHI
{
    CommandList::CommandList(const CommandListCreateInfo& createInfo)
    : mCommandBuffer(createInfo.commandBuffer)
    , mSingleTime(createInfo.singleTimeSubmit)
    , mDebug(Configuration::getConfig().rhi.debugFeatures)
    {
    }

    void CommandList::begin() noexcept
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

    void CommandList::end() noexcept
    {
        assert(mIsRecording);
        mIsRecording = false;

        mCommandBuffer.end();
    }

    void CommandList::beginLabel(std::string_view label) const noexcept
    {
        if (!mDebug)
        {
            return;
        }

        const auto debugLabel = vk::DebugUtilsLabelEXT()
            .setPLabelName(label.data());
        mCommandBuffer.beginDebugUtilsLabelEXT(debugLabel);
    }

    void CommandList::beginLabel(std::string_view label, const std::array<float, 3>& color) const noexcept
    {
        if (!mDebug)
        {
            return;
        }

        const auto debugLabel = vk::DebugUtilsLabelEXT()
            .setColor({ color[0], color[1], color[2], 1.0f })
            .setPLabelName(label.data());
        mCommandBuffer.beginDebugUtilsLabelEXT(debugLabel);
    }

    void CommandList::endLabel() const noexcept
    {
        if (!mDebug)
        {
            return;
        }

        mCommandBuffer.endDebugUtilsLabelEXT();
    }

    void CommandList::copyBufferToImage(const BufferImageCopyInfo& copyInfo) const noexcept
    {
        const auto imageProperties = copyInfo.pDstImage->getProperties();
        const auto copyRegion = vk::BufferImageCopy2()
            .setBufferOffset(0)
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

    void CommandList::copyBuffer(const CopyBufferInfo& copyInfo) const noexcept
    {
        mCommandBuffer.copyBuffer2(copyInfo.vk());
    }

    void CommandList::setScissor(const Rect2D& scissor) const noexcept
    {
        mCommandBuffer.setScissor(0, scissor.vk());
    }

    void CommandList::setViewport(const Viewport& viewport) const noexcept
    {
        mCommandBuffer.setViewport(0, viewport.vk());
    }

    void CommandList::beginRendering() const noexcept
    {
        // TODO: no-op for now
    }

    void CommandList::endRendering() const noexcept
    {
        mCommandBuffer.endRendering();
    }

    void CommandList::bindPipeline(Pipeline* pPipeline) const noexcept
    {
        mCommandBuffer.bindPipeline(pPipeline->getBindPoint(), pPipeline->getHandle());
    }

    void CommandList::draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex,
        const uint32_t firstInstance) const noexcept
    {
        mCommandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void CommandList::drawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex,
        const uint32_t vertexOffset, const uint32_t firstInstance) const noexcept
    {
        mCommandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void CommandList::bindVertexBuffers(const uint32_t firstBinding, const std::vector<Buffer*>& buffers,
        const std::vector<DeviceSize>& offsets) const noexcept
    {
        std::vector<vk::Buffer> bufferHandles;
        for (const auto* pBuffer : buffers)
        {
            bufferHandles.push_back(pBuffer->getHandle());
        }
        mCommandBuffer.bindVertexBuffers(firstBinding, buffers.size(), bufferHandles.data(), offsets.data());
    }

    void CommandList::bindIndexBuffer(Buffer* pBuffer, const DeviceSize offset,
        const IndexType indexType) const noexcept
    {
        mCommandBuffer.bindIndexBuffer(pBuffer->getHandle(), offset, to_vk(indexType));
    }

    void CommandList::insertBarrier(const DependencyInfo& dependencyInfo) const noexcept
    {
        auto dependencies = Barrier();
        for (const auto& [pImage, dstUsage] : dependencyInfo.imageMemoryBarriers)
        {
            dependencies.addBarrier(pImage->getBarrier(dstUsage));
        }
        dependencies.insert(mCommandBuffer);
    }
}
