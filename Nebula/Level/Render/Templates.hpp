#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>
#include "Core/Types.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/Rendering/RenderPass.hpp"

namespace nbl
{
    /**
     * Create a render target with Transfer, Sampled, Storage and Color/Depth usage deduced from format.
     * @param pRHI
     * @param name Debug label
     * @param format Default: RGBA16 float
     * @param extent If no extent is specified, the current swapchain size is used for creation.
     * @param samplerInfo
     * @return Image
     */
    [[nodiscard]] inline SPtr<RHI::Image> makeRenderTarget(
        const RHI::VulkanRHI*                       pRHI,
        const std::string&                          name,
        const vk::Format                            format = vk::Format::eR16G16B16A16Sfloat,
        const std::optional<vk::Extent2D>&          extent = std::nullopt,
        const std::optional<vk::SamplerCreateInfo>& samplerInfo = std::nullopt
        )
    {
        const auto isDepth = vk::hasDepthComponent(format);

        using enum vk::ImageUsageFlagBits;
        const vk::ImageUsageFlags usageFlags = (isDepth ? eDepthStencilAttachment : eColorAttachment | eStorage)
            | eTransferSrc | eTransferDst | eSampled;

        const auto _extent = extent.value_or(pRHI->getSwapchain()->getProperties().extent);

        return pRHI->createImage({
            .extent        = _extent,
            .format        = format,
            .usageFlags    = usageFlags,
            .createSampler = true,
            .debugName     = name,
            .samplerInfo   = samplerInfo,
        });
    }

    /**
     * Create a rendering attachment, automatically resolving color/depth usage.
     * @param pImage Image to use as attachment
     * @param loadOp Default: Clear
     * @param storeOp Default: Store
     * @param clearValue Default: (1.0f, 0) for depth (0.0f RGB 1.0f A) for color
     * @param pResolve Image to use as resolve target when pImage is multisampled
     * @return RHI Attachment description
     */
    [[nodiscard]] inline RHI::Attachment makeAttachment(
        const SPtr<RHI::Image>&              pImage,
        const vk::AttachmentLoadOp           loadOp     = vk::AttachmentLoadOp::eClear,
        const vk::AttachmentStoreOp          storeOp    = vk::AttachmentStoreOp::eStore,
        const std::optional<vk::ClearValue>& clearValue = std::nullopt,
        const SPtr<RHI::Image>&              pResolve = nullptr)
    {
        const auto isDepth = vk::hasDepthComponent(pImage->getProperties().format);
        const auto _clearValue = clearValue.value_or(isDepth
            ? vk::ClearValue().setDepthStencil({1.0f, 0})
            : vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}));
        const auto _layout = isDepth
            ? vk::ImageLayout::eDepthAttachmentOptimal
            : vk::ImageLayout::eColorAttachmentOptimal;

        auto attachmentInfo = vk::RenderingAttachmentInfo()
            .setClearValue(_clearValue)
            .setImageLayout(_layout)
            .setImageView(pImage->getImageView())
            .setLoadOp(loadOp)
            .setStoreOp(pResolve ? vk::AttachmentStoreOp::eDontCare : storeOp);

        if (pResolve)
        {
            attachmentInfo
                .setResolveImageLayout(_layout)
                .setResolveImageView(pResolve->getImageView())
                .setResolveMode(isDepth
                    ? vk::ResolveModeFlagBits::eSampleZero
                    : vk::ResolveModeFlagBits::eAverage);
        }

        return {
            .image = pImage->getImage(),
            .attachmentInfo = std::move(attachmentInfo),
        };
    }

    [[nodiscard]] inline RHI::Attachment makeResolvedAttachment(const SPtr<RHI::Image>& pMultisampled, const SPtr<RHI::Image>& pResolve)
    {
        return makeAttachment(
            pMultisampled,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            std::nullopt,
            pResolve);
    }

    [[nodiscard]] inline vk::Rect2D getRenderAreaForAttachment(const RHI::Image* pImage)
    {
        return vk::Rect2D()
            .setOffset({ 0, 0 })
            .setExtent(pImage->getProperties().extent);
    }
}
