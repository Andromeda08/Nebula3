#include "CommandList.hpp"

#include "Core/Random.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/Swapchain.hpp"
#include "VulkanRHI/Render/Pipeline.hpp"

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

    void CommandList::draw(
        const uint32_t vertexCount, const uint32_t instanceCount,
        const uint32_t firstVertex, const uint32_t firstInstance
    ) const
    {
        exitOnAssert(mBoundPipeline && mBoundPipeline->getType() == PipelineType2::Graphics, "No graphics pipeline is bound.");
        mCommandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void CommandList::drawIndexed(
        const uint32_t indexCount,   const uint32_t instanceCount, const uint32_t firstIndex,
        const int32_t  vertexOffset, const uint32_t firstInstance
    ) const
    {
        exitOnAssert(mBoundPipeline && mBoundPipeline->getType() == PipelineType2::Graphics, "No graphics pipeline is bound.");
        mCommandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void CommandList::drawIndexedIndirect(
        const Buffer*  pBuffer,   const uint64_t offset,
        const uint32_t drawCount, const uint32_t stride
    ) const
    {
        exitOnAssert(mBoundPipeline && mBoundPipeline->getType() == PipelineType2::Graphics, "No graphics pipeline is bound.");
        mCommandBuffer.drawIndexedIndirect(pBuffer->getHandle(), offset, drawCount, stride);
    }

    void CommandList::bindPipeline(PipelineBase* pPipeline)
    {
        mBoundPipeline = pPipeline;
        mCommandBuffer.bindPipeline(pPipeline->getBindPoint(), pPipeline->getHandle());
    }

    void CommandList::bindDescriptorSet(const vk::DescriptorSet& descriptorSet, const uint32_t setIndex) const
    {
        exitOnAssert(mBoundPipeline, "No pipeline is bound.");
        mCommandBuffer.bindDescriptorSets(
            mBoundPipeline->getBindPoint(),
            mBoundPipeline->getLayout(),
            setIndex,
            1, &descriptorSet,
            0, nullptr);
    }

    void CommandList::pushConstants(const void* pData) const
    {
        exitOnAssert(mBoundPipeline, "No pipeline is bound.");
        if (const auto& pcr = mBoundPipeline->getPushConstantRange(); pcr.has_value())
        {
            mCommandBuffer.pushConstants(
                mBoundPipeline->getLayout(),
                pcr->stageFlags,
                pcr->offset,
                pcr->size,
                pData);
        }
    }

    void CommandList::dispatch(const uint32_t x, const uint32_t y, const uint32_t z) const
    {
        exitOnAssert(mBoundPipeline && mBoundPipeline->getType() == PipelineType2::Compute, "No compute pipeline is bound.");
        mCommandBuffer.dispatch(x, y, z);
    }

    void CommandList::dispatchIndirect(const Buffer* pBuffer, uint64_t offset) const
    {
        exitOnAssert(mBoundPipeline && mBoundPipeline->getType() == PipelineType2::Compute, "No compute pipeline is bound.");
        mCommandBuffer.dispatchIndirect(pBuffer->getHandle(), 0);
    }

    void CommandList::traceRays(const uint32_t x, const uint32_t y, const uint32_t rayDepth) const
    {
        exitOnAssert(mBoundPipeline && mBoundPipeline->getType() == PipelineType2::RayTracing, "No ray tracing pipeline is bound.");

        const auto* rt  = dynamic_cast<RayTracingPipeline2*>(mBoundPipeline);
        const auto* sbt = rt->getShaderBindingTable();

        const uint32_t depth = std::min(rayDepth, rt->getMaxDepth());
        if (rayDepth > rt->getMaxDepth())
        {
            spdlog::warn("The specified depth exceeds the maximum of the currently bound ray tracing pipeline.");
        }

        mCommandBuffer.traceRaysKHR(
            sbt->getRaygenRegion(), sbt->getMissRegion(), sbt->getHitRegion(), sbt->getCallRegion(),
            x, y, depth);
    }

    void CommandList::beginLabel(const std::array<float, 3>& color, const std::string& name) const
    {
        beginLabel(name, color);
    }

    void CommandList::beginLabel(const std::string& label, const std::array<float, 3>& color) const
    {
        if (!mDebug)
        {
            return;
        }

        const auto labelInfo = vk::DebugUtilsLabelEXT()
            .setColor({ color[0], color[1], color[2], 1.0f })
            .setPLabelName(label.c_str());
        mCommandBuffer.beginDebugUtilsLabelEXT(labelInfo);
    }

    void CommandList::beginLabel(const std::string& name) const
    {
        if (!mDebug)
        {
            return;
        }

        const auto label = vk::DebugUtilsLabelEXT()
            .setColor({ Random::unit(), Random::unit(), Random::unit(), 1.0f })
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

    void CommandList::blitToSwapchain(Image* pSrcImage, Swapchain* pSwapchain, const uint32_t acquiredIndex) const
    {
        beginLabel(fmt::format("blitToSwapchain_{}", acquiredIndex));
        
        Barrier()
            .addBarrier(pSrcImage->getBarrier(ImageUsage::TransferSrc))
            .addBarrier(pSwapchain->getBarrier(acquiredIndex, ImageUsage::TransferDst))
            .insert(mCommandBuffer);
        
        #pragma region "Blit setup"
        
        const auto srcExtent = pSrcImage->getProperties().extent;
        const auto dstExtent = pSwapchain->getProperties().extent;
        const auto region  = vk::ImageBlit2()
            .setSrcOffsets({
                vk::Offset3D { 0, 0, 0 },
                vk::Offset3D { static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), 1 }
            })
            .setSrcSubresource(pSrcImage->getProperties().getSubresourceLayers())
            .setDstOffsets({
                vk::Offset3D { 0, 0, 0 },
                vk::Offset3D { static_cast<int32_t>(dstExtent.width), static_cast<int32_t>(dstExtent.height), 1 }
            })
            .setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });

        const auto blit = vk::BlitImageInfo2()
            .setSrcImage(pSrcImage->getImage())
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setDstImage(pSwapchain->getImage(acquiredIndex))
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setFilter(vk::Filter::eLinear)
            .setRegions(region);
        
        #pragma endregion
        
        mCommandBuffer.blitImage2(blit);
        
        endLabel();
    }

    void CommandList::setViewportScissor(const vk::Viewport& viewport, const vk::Rect2D& scissor) const
    {
        mCommandBuffer.setScissor(0, scissor);
        mCommandBuffer.setViewport(0, viewport);
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
