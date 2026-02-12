#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "Instance.hpp"
#include "../RHI/IWindow.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "Core/Platform.hpp"
#include "Detail/ImageTraits.hpp"

namespace RHI
{
    class Image;

    struct SwapchainCreateInfo
    {
        const SPtr<IWindow>&  window;
        const SPtr<Instance>& instance;
        const SPtr<Device>&   device;

        vk::Format            prefFormat      = vk::Format::eB8G8R8A8Unorm;
        vk::ColorSpaceKHR     prefColorSpace  = vk::ColorSpaceKHR::eSrgbNonlinear;
        vk::PresentModeKHR    prefPresentMode = platform::isApple ? vk::PresentModeKHR::eImmediate : vk::PresentModeKHR::eMailbox;
    };

    struct SwapchainProperties
    {
        vk::Extent2D                    extent              = {0, 0};
        float                           aspectRatio         = 0.0f;
        vk::Rect2D                      area                = {{0,0}, {0,0}};
        vk::Format                      format              = vk::Format::eB8G8R8A8Unorm;
        vk::ColorSpaceKHR               colorSpace          = vk::ColorSpaceKHR::eSrgbNonlinear;
        vk::PresentModeKHR              presentMode         = vk::PresentModeKHR::eMailbox;
        vk::SurfaceTransformFlagBitsKHR currentTransform    = {};
    };

    class Swapchain
    {
    public:
        nbl_DISABLE_COPY(Swapchain);
        nbl_CTOR(Swapchain);

        ~Swapchain();

        void setScissorViewport(const vk::CommandBuffer& commandList) const;

        /**
         * Create an ImageMemoryBarrier to the dstState for the specified image.
         * @param i Swapchain image index
         * @param dstUsage
         * @param update Update the internally tracked state
         */
        vk::ImageMemoryBarrier2 getBarrier(const uint32_t i, const ImageUsage& dstUsage, const bool update = true) noexcept
        {
            assert(i < mImages.size());
            auto& imageState = mImageStates[i];

            const auto dstState = getImageState(dstUsage);
            const auto barrier  = makeImageMemoryBarrier(mImageStates[i], dstState)
                .setImage(mImages[i])
                .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

            if (update)
            {
                imageState = dstState;
            }

            return barrier;
        }

        vk::Image getImage(size_t i) const;

        vk::ImageView getImageView(size_t i) const;

        const SwapchainProperties& getProperties() const noexcept
        {
            return mProperties;
        }

        uint64_t getImageCount() const noexcept
        {
            return mImageCount;
        }

        vk::SwapchainKHR getHandle() const { return mSwapchain; }

    private:
        void initAndValidateProperties(const SwapchainCreateInfo& info);
        void acquireImages();
        void makeScissorViewport();

        const uint64_t                  mImageCount        = gFramesInFlight;
        uint32_t                        mLastAcquiredIndex = 0;

        SwapchainProperties             mProperties {};
        vk::SurfaceKHR                  mSurface;
        vk::SwapchainKHR                mSwapchain;
        vk::Rect2D                      mScissor;
        vk::Viewport                    mViewport;

        PerFrameArray<vk::Image>        mImages;
        PerFrameArray<vk::ImageView>    mImageViews;
        PerFrameArray<ImageState>       mImageStates;

        SPtr<IWindow>                   mWindow;
        SPtr<Instance>                  mInstance;
        SPtr<Device>                    mDevice;
    };
}
