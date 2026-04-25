#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>
#include "Core/Types.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/Rendering/RenderPass.hpp"

namespace nbl
{
    /**
     * Create a rendering attachment, automatically resolving color/depth usage.
     * @param pImage Image to use as attachment
     * @param loadOp Default: Clear
     * @param storeOp Default: Store
     * @param clearValue Default: (1.0f, 0) for depth (0.0f RGB 1.0f A) for color
     * @return RHI Attachment description
     */
    [[nodiscard]] inline RHI::Attachment makeAttachment(
        const RHI::Image*                    pImage,
        const vk::AttachmentLoadOp           loadOp     = vk::AttachmentLoadOp::eClear,
        const vk::AttachmentStoreOp          storeOp    = vk::AttachmentStoreOp::eStore,
        const std::optional<vk::ClearValue>& clearValue = std::nullopt)
    {
        const auto isDepth = vk::hasDepthComponent(pImage->getProperties().format);
        const auto _clearValue = clearValue.value_or(isDepth
            ? vk::ClearValue().setDepthStencil({1.0f, 0})
            : vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}));
        const auto _layout = isDepth
            ? vk::ImageLayout::eDepthAttachmentOptimal
            : vk::ImageLayout::eColorAttachmentOptimal;

        return {
            .image = pImage->getImage(),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(_clearValue)
                .setImageLayout(_layout)
                .setImageView(pImage->getImageView())
                .setLoadOp(loadOp)
                .setStoreOp(storeOp)
        };
    }

    [[nodiscard]] inline vk::Rect2D getRenderAreaForAttachment(const RHI::Image* pImage)
    {
        return vk::Rect2D()
            .setOffset({ 0, 0 })
            .setExtent(pImage->getProperties().extent);
    }
}
