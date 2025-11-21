#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "Instance.hpp"
#include "IWindow.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"

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
        vk::PresentModeKHR    prefPresentMode = vk::PresentModeKHR::eMailbox;
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

        vk::SwapchainKHR getHandle() const { return mSwapchain; }

        SPtr<Image>   getImage(size_t i) const;
        vk::Image     getImageHandle(size_t i) const;
        vk::ImageView getImageView(size_t i)   const;

        const SwapchainProperties& getProperties() const noexcept
        {
            return mProperties;
        }

        uint64_t getImageCount() const noexcept
        {
            return mImageCount;
        }

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
        PerFrameArray<SPtr<Image>>      mWrappedImages;

        SPtr<IWindow>                   mWindow;
        SPtr<Instance>                  mInstance;
        SPtr<Device>                    mDevice;
    };
}
