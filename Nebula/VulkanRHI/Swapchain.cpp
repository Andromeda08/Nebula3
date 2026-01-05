#include "Swapchain.hpp"

#include <limits>

#include "Image.hpp"

namespace RHI
{
    Swapchain::Swapchain(const SwapchainCreateInfo& createInfo)
    : mWindow(createInfo.window)
    , mInstance(createInfo.instance)
    , mDevice(createInfo.device)
    {
        mWindow->createVulkanSurface(mInstance->getHandle(), &mSurface);

        initAndValidateProperties(createInfo);

        const auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
            .setSurface(mSurface)
            .setMinImageCount(mImageCount)
            .setImageFormat(mProperties.format)
            .setImageColorSpace(mProperties.colorSpace)
            .setImageExtent(mProperties.extent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
            .setPreTransform(mProperties.currentTransform)
            .setClipped(true)
            .setOldSwapchain(nullptr)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setPresentMode(mProperties.presentMode)
            .setQueueFamilyIndexCount(0)
            .setPQueueFamilyIndices(nullptr);

        mSwapchain = mDevice->getHandle().createSwapchainKHR(swapchainCreateInfo);

        acquireImages();
        makeScissorViewport();
    }

    Swapchain::~Swapchain()
    {
        for (const auto& imageView : mImageViews)
        {
            mDevice->getHandle().destroyImageView(imageView);
        }

        mDevice->getHandle().destroySwapchainKHR(mSwapchain);

        mInstance->getHandle().destroySurfaceKHR(mSurface);
    }

    void Swapchain::setScissorViewport(const vk::CommandBuffer& commandList) const
    {
        commandList.setScissor(0, 1, &mScissor);
        commandList.setViewport(0, 1, &mViewport);
    }

    vk::Image Swapchain::getImage(const size_t i) const
    {
        assert(i < mImages.size());
        return mImages[i];
    }

    vk::ImageView Swapchain::getImageView(const size_t i) const
    {
        assert(i < mImages.size());
        return mImageViews[i];
    }

    void Swapchain::initAndValidateProperties(const SwapchainCreateInfo& info)
    {
        const auto physicalDevice = mDevice->getPhysicalDevice();

        const auto surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(mSurface);

        mProperties.currentTransform = surfaceCaps.currentTransform;

        // Image Count must be withing [min, max]
        const auto imageCountOk = !(surfaceCaps.minImageCount > mImageCount || surfaceCaps.maxImageCount < mImageCount);
        assert(imageCountOk);

        // Extent
        if (surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            mProperties.extent = vk::Extent2D()
                .setWidth(surfaceCaps.currentExtent.width)
                .setHeight(surfaceCaps.currentExtent.height);
        }

        const vk::Extent2D min = surfaceCaps.minImageExtent;
        const vk::Extent2D max = surfaceCaps.maxImageExtent;

        auto [width, height] = mWindow->getFramebufferSize();
        mProperties.extent.width = std::clamp(width, min.width, max.width);
        mProperties.extent.height = std::clamp(height, min.height, max.height);

        mProperties.area = vk::Rect2D()
            .setExtent(mProperties.extent)
            .setOffset({ 0, 0 });

        // Surface Format & ColorSpace
        const auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(mSurface);
        assert(!surfaceFormats.empty());

        bool hasPreferredFormat = false;
        for (const auto& surfaceFormat : surfaceFormats)
        {
            if (surfaceFormat.format == info.prefFormat && surfaceFormat.colorSpace == info.prefColorSpace)
            {
                hasPreferredFormat = true;
            }
        }

        if (hasPreferredFormat)
        {
            mProperties.format     = info.prefFormat;
            mProperties.colorSpace = info.prefColorSpace;
        }
        else
        {
            mProperties.format     = surfaceFormats[0].format;
            mProperties.colorSpace = surfaceFormats[0].colorSpace;
        }

        // Present Mode
        const auto presentModes = physicalDevice.getSurfacePresentModesKHR(mSurface);
        assert(!presentModes.empty());

        const auto presentModeIt = std::ranges::find(presentModes, info.prefPresentMode);
        mProperties.presentMode = (presentModeIt != std::end(presentModes)) ? info.prefPresentMode : presentModes[0];
    }

    void Swapchain::acquireImages()
    {
        vk::Result result {};

        auto imageCount = static_cast<uint32_t>(mImageCount);
        result = mDevice->getHandle().getSwapchainImagesKHR(mSwapchain, &imageCount, mImages.data());
        assert(result == vk::Result::eSuccess);

        constexpr vk::ComponentMapping componentMapping = {
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity
        };

        auto viewCreateInfo = vk::ImageViewCreateInfo()
            .setComponents(componentMapping)
            .setFormat(mProperties.format)
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 })
            .setViewType(vk::ImageViewType::e2D);

        for (size_t i = 0; i < mImageViews.size(); i++)
        {
            viewCreateInfo.setImage(mImages[i]);
            result = mDevice->getHandle().createImageView(&viewCreateInfo, nullptr, &mImageViews[i]);
            assert(result == vk::Result::eSuccess);

            mDevice->nameObject<vk::Image>({
                .debugName = std::format("SwapchainImage[{}]", i),
                .handle    = mImages[i],
            });

            mDevice->nameObject<vk::ImageView>({
                .debugName = std::format("SwapchainImage[{}]", i),
                .handle    = mImageViews[i],
            });
        }
    }

    void Swapchain::makeScissorViewport()
    {
        const auto& extent = mProperties.extent;
        mScissor = vk::Rect2D {{ 0, 0 }, extent};

        /**
         * Create a viewport object based on the current state of the Swapchain.
         * The viewport is flipped along the Y axis for GLM compatibility.
         * This requires Maintenance1, which is core Vulkan since API version 1.1.
         * https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
         */
        mViewport = vk::Viewport()
            .setX(0.0f)
            .setWidth(static_cast<float>(extent.width))
            .setY(static_cast<float>(extent.height))
            .setHeight(-1.0f * static_cast<float>(extent.height))
            .setMaxDepth(1.0f)
            .setMinDepth(0.0f);
    }
}
