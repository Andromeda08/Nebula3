#pragma once

#include <rhi/vulkan/Device.hpp>
#include <rhi/vulkan/VulkanCommon.hpp>
#include <rhi/vulkan/detail/Surface.hpp>

#include "Texture.hpp"

namespace sunflower::rhi
{
    struct VulkanSwapchainCreateInfo
    {
        IWindow*         pWindow;
        detail::Surface* pSurface;
        SPtr<Device>     device;
    };

    class VulkanSwapchain final : public Swapchain
    {
    public:
        sunflower_DisableCopy(VulkanSwapchain);
        sunflower_Create(VulkanSwapchain, UPtr);

        ~VulkanSwapchain() override;

        [[nodiscard]] const SPtr<Texture>& getTexture(uint64_t i) const override;

        [[nodiscard]] Format getFormat() const noexcept override;

        [[nodiscard]] Size getSize() const noexcept override;

        [[nodiscard]] const vk::SwapchainKHR& getHandle() const noexcept;

    private:
        static constexpr auto sPrefPresentMode = conf::gIsApple ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
        static constexpr auto sPrefColorSpace  = vk::ColorSpaceKHR::eSrgbNonlinear;

        IWindow*                    mWindow;
        detail::Surface*            mSurface;
        SPtr<Device>                mDevice;

        vk::SwapchainKHR            mSwapchain;

        std::vector<vk::Image>      mAcquiredImages;
        std::vector<SPtr<Texture>>  mTextures;

        Format                      mFormat = gSwapchainFormat;
        Size                        mSize   = {};
    };
}
