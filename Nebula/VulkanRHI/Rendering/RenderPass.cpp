#include "RenderPass.hpp"

#include <ranges>

namespace RHI
{
    RenderPass::RenderPass(const RenderPassCreateInfo& createInfo)
    : mRenderArea(createInfo.renderArea)
    , mDepthAttachment(createInfo.depthAttachment)
    {
        mColorRenderingInfos = createInfo.colorAttachments
            | std::views::transform([](const auto& x){ return x.attachmentInfo; })
            | std::ranges::to<std::vector<vk::RenderingAttachmentInfo>>();

        mRenderingInfo = vk::RenderingInfo()
            .setRenderArea(mRenderArea)
            .setLayerCount(1)
            .setPColorAttachments(mColorRenderingInfos.data())
            .setColorAttachmentCount(mColorRenderingInfos.size())
            .setPDepthAttachment(&mDepthAttachment.attachmentInfo);
    }

    void RenderPass::execute(const vk::CommandBuffer& commandList, const std::function<void(const vk::CommandBuffer&)>& lambda) const
    {
        std::vector<vk::ImageMemoryBarrier2> imageBarriers;

        std::array<std::span<const Attachment>, 2> views = { std::span{mColorAttachments}, std::span<Attachment>{} };
        if (mDepthAttachment.image)
        {
            views[1] = std::span{&mDepthAttachment, 1};
        }

        for (const auto& attachment : views | std::views::join)
        {
            auto newLayout = vk::ImageLayout::eColorAttachmentOptimal;
            if (isDepthFormat(attachment.image->getProperties().format))
            {
                newLayout = vk::ImageLayout::eDepthAttachmentOptimal;
            }

            const auto barrier = vk::ImageMemoryBarrier2()
                .setImage(attachment.image->getImage())
                .setOldLayout(attachment.image->getState().layout)
                .setNewLayout(newLayout)
                .setSrcAccessMask(attachment.image->getState().accessFlags)
                .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands);

            imageBarriers.push_back(barrier);
        }

        const auto dependencyInfo = vk::DependencyInfo()
            .setImageMemoryBarrierCount(imageBarriers.size())
            .setPImageMemoryBarriers(imageBarriers.data());

        commandList.pipelineBarrier2(dependencyInfo);
        commandList.beginRendering(&mRenderingInfo);
        lambda(commandList);
        commandList.endRendering();

        for (auto& barrier : imageBarriers)
        {
            std::swap(barrier.oldLayout, barrier.newLayout);
            std::swap(barrier.srcAccessMask, barrier.dstAccessMask);
            std::swap(barrier.srcStageMask, barrier.dstStageMask);
        }
        commandList.pipelineBarrier2(dependencyInfo);
    }

    void RenderPass::setColorAttachment(const uint32_t index, const Attachment& attachment)
    {
        assert(index < mColorAttachments.size());
        mColorAttachments[index] = attachment;
        mColorRenderingInfos[index] = attachment.attachmentInfo;
    }
}
