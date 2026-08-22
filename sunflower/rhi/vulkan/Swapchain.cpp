#include "Swapchain.hpp"

namespace sunflower::rhi
{
    VulkanSwapchain::VulkanSwapchain(const VulkanSwapchainCreateInfo& createInfo)
    : mWindow(createInfo.pWindow)
    , mSurface(createInfo.pSurface)
    , mDevice(createInfo.device)
    {
        auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
            .setClipped(true)
            .setImageArrayLayers(1)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst)
            .setOldSwapchain(nullptr)
            .setPQueueFamilyIndices(nullptr)
            .setQueueFamilyIndexCount(0)
            .setSurface(mSurface->getHandle());

        const auto surfaceCaps = mSurface->getCapabilities(mDevice->getPhysicalDevice());
        swapchainCreateInfo.setPreTransform(surfaceCaps.currentTransform);

        // Image count
        if (!isInRange(conf::gFramesInFlight, surfaceCaps.minImageCount, surfaceCaps.maxImageCount))
        {
            ::sunflower::exit("Unsupported image count of {}. [min={}, max={}]", conf::gFramesInFlight, surfaceCaps.minImageCount, surfaceCaps.maxImageCount);
        }
        swapchainCreateInfo.setMinImageCount(conf::gFramesInFlight);

        // Extent (clamped)
        const auto& [width, height, _] = mWindow->getFramebufferSize();
        mSize = {
            .width  = std::clamp(width,  surfaceCaps.minImageExtent.width,  surfaceCaps.maxImageExtent.width),
            .height = std::clamp(height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height),
            .depth  = 1u,
        };
        swapchainCreateInfo.setImageExtent({ mSize.width, mSize.height });

        // Format
        const auto format = mSurface->getFormat(mDevice->getPhysicalDevice(), gSwapchainFormat, sPrefColorSpace);
        swapchainCreateInfo.setImageFormat(format.format);
        swapchainCreateInfo.setImageColorSpace(format.colorSpace);

        // Present Mode
        const auto presentMode = mSurface->getPresentMode(mDevice->getPhysicalDevice(), sPrefPresentMode);
        swapchainCreateInfo.setPresentMode(presentMode);

        // Create
        {
            const auto [result, swapchain] = mDevice->getHandle().createSwapchainKHR(swapchainCreateInfo);
            if (result != vk::Result::eSuccess)
            {
                ::sunflower::exit("Failed to create swapchain: {}", vk::to_string((result)));
            }
            mSwapchain = swapchain;
        }

        // Get and wrap images
        {
            const auto [result, images] = mDevice->getHandle().getSwapchainImagesKHR(mSwapchain);
            if (result != vk::Result::eSuccess)
            {
                ::sunflower::exit("Failed to get swapchain images: {}", vk::to_string((result)));
            }
            mAcquiredImages = images;

            mTextures.resize(mAcquiredImages.size());
            for (const auto& [i, image] : enumerate(mAcquiredImages))
            {
                mTextures[i] = VulkanTexture::createWrapped({
                    .pSwapchain = this,
                    .handle     = image,
                    .label      = fmt::format("SwapchainImage-{}-", i),
                }, mDevice);
            }
        }
    }

    VulkanSwapchain::~VulkanSwapchain()
    {
        mDevice->getHandle().destroy(mSwapchain);
    }

    Texture* VulkanSwapchain::getTexture(const uint64_t i) const
    {
        return mTextures[i].get();
    }

    Format VulkanSwapchain::getFormat() const noexcept
    {
        return mFormat;
    }

    Size VulkanSwapchain::getSize() const noexcept
    {
        return mSize;
    }

    const vk::SwapchainKHR& VulkanSwapchain::getHandle() const noexcept
    {
        return mSwapchain;
    }
}
