#include "Texture.hpp"

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_format_traits.hpp>

namespace sunflower::rhi
{
    namespace detail
    {
        [[nodiscard]] static constexpr vk::ImageAspectFlags getImageAspectFlags(const vk::Format format) noexcept
        {
            vk::ImageAspectFlags aspectFlags = {};

            if (vk::isColor(format))
            {
                aspectFlags = vk::ImageAspectFlagBits::eColor;
            }
            if (vk::hasDepthComponent(format))
            {
                aspectFlags |= vk::ImageAspectFlagBits::eDepth;
            }
            if (vk::hasStencilComponent(format))
            {
                aspectFlags |= vk::ImageAspectFlagBits::eStencil;
            }

            return aspectFlags;
        }

    }

    VulkanTexture::VulkanTexture(const TextureCreateInfo& createInfo, const SPtr<Device>& device)
    : mDevice(device)
    , mFormat(createInfo.format)
    , mSize(createInfo.size)
    , mType(createInfo.type)
    , mMipLevels(getMipLevels(createInfo, false))
    , mLayers(getTextureLayerCount(createInfo))
    {
        createImage(createInfo);
        createImageView();
    }

    // TODO: Setup image metadata from Swapchain
    VulkanTexture::VulkanTexture(const VulkanSwapchainWrappedTextureCreateInfo& createInfo, const SPtr<Device>& device)
    : mDevice(device)
    , mSwapchain(createInfo.pSwapchain)
    , mImage(createInfo.handle)
    {
        if (mImage == VK_NULL_HANDLE)
        {
            ::sunflower::exit("Invalid Image handle specified to wrapped texture ctor.");
        }
        createImageView(createInfo.label);
    }

    VulkanTexture::~VulkanTexture()
    {
        if (!mSwapchain)
        {
            mDevice->getHandle().destroy(mImageView);
            vmaDestroyImage(mDevice->getAllocator(), mImage, mAlloc);
        }
    }

    Format VulkanTexture::getFormat() const noexcept
    {
        return mFormat;
    }

    const Size& VulkanTexture::getSize() const noexcept
    {
        return mSize;
    }

    const vk::Image& VulkanTexture::getHandle() const noexcept
    {
        return mImage;
    }

    const vk::ImageView& VulkanTexture::getImageView() const noexcept
    {
        return mImageView;
    }

    vk::ImageSubresourceRange VulkanTexture::getSubresourceRange() const noexcept
    {
        return {
            detail::getImageAspectFlags(toVulkan(mFormat)),
            0, mMipLevels,
            0, mLayers,
        };
    }

    bool VulkanTexture::isWrappedImage() const noexcept
    {
        return mSwapchain != nullptr;
    }

    void VulkanTexture::createImage(const TextureCreateInfo& createInfo)
    {
        auto imageCreateInfo = vk::ImageCreateInfo()
            .setArrayLayers(mLayers)
            .setExtent({ mSize.width, mSize.height, mSize.depth })
            .setFormat(toVulkan(mFormat))
            .setImageType(toVulkan_ImageType(mType))
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setMipLevels(mMipLevels)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(toVulkan(createInfo.usage));

        if (mType == TextureType::eCube)
        {
            imageCreateInfo.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
        }

        constexpr VmaAllocationCreateInfo allocCreateInfo = {
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        const auto result = vmaCreateImage(
            mDevice->getAllocator(),
            &static_cast<const VkImageCreateInfo&>(imageCreateInfo),
            &allocCreateInfo,
            reinterpret_cast<VkImage*>(&mImage),
            &mAlloc,
            &mAllocInfo);

        if (result != VK_SUCCESS)
        {
            ::sunflower::exit("Failed to create Image: {}", string_VkResult(result));
        }

        mDevice->setLabel(mImage, createInfo.label);
    }

    void VulkanTexture::createImageView(const Option<String>& label)
    {
        const auto imageViewCreateInfo = vk::ImageViewCreateInfo()
            .setFormat(toVulkan(mFormat))
            .setImage(mImage)
            .setSubresourceRange(getSubresourceRange())
            .setViewType(toVulkan_ImageViewType(mType));

        const auto [result, imageView] = mDevice->getHandle().createImageView(imageViewCreateInfo);
        if (result != vk::Result::eSuccess)
        {
            exit("Failed to create ImageView: {}", vk::to_string(result));
        }
        mImageView = imageView;

        mDevice->setLabel(mImageView, label.transform([](const auto& s){ return s + "View"; }));
    }
}
