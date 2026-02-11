#include "RenderPass.hpp"

#include <ranges>

#include "VulkanRHI/Commands/CommandList.hpp"

namespace RHI
{
    RenderPass::RenderPass(const RenderPassCreateInfo& createInfo)
    : mRenderArea(createInfo.renderArea.vk())
    , mColorAttachments(createInfo.colorAttachments)
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
        commandList.beginRendering(&mRenderingInfo);
        lambda(commandList);
        commandList.endRendering();
    }

    void RenderPass::execute(const CommandList* pCommandList, const std::function<void(const CommandList*)>& lambda) const
    {
        // TODO: Handleless begin rendering
        pCommandList->getHandle().beginRendering(mRenderingInfo);
        lambda(pCommandList);
        pCommandList->endRendering();
    }

    void RenderPass::setColorAttachment(const uint32_t index, const Attachment& attachment)
    {
        assert(index < mColorAttachments.size());
        mColorAttachments[index] = attachment;
        mColorRenderingInfos[index] = attachment.attachmentInfo;
    }
}
