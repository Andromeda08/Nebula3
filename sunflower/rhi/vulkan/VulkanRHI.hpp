#pragma once

#include <rhi/DynamicRHI.hpp>
#include <rhi/IWindow.hpp>

#include <rhi/vulkan/Device.hpp>
#include <rhi/vulkan/VulkanCommon.hpp>
#include <rhi/vulkan/detail/Instance.hpp>
#include <rhi/vulkan/detail/Surface.hpp>

#include "Swapchain.hpp"

/**
 * Vulkan RHI targeting bindless-first GPU-driven setups.
 */
namespace sunflower::rhi
{
    struct VulkanRHICreateInfo
    {
        IWindow*        pWindow         = nullptr;
        Option<String>  applicationName = {};
        Option<String>  engineName      = {};
    };

    class VulkanRHI final : public DynamicRHI
    {
    public:
        sunflower_DisableCopy(VulkanRHI);
        sunflower_Create(VulkanRHI, SPtr)

        [[nodiscard]] SPtr<Texture> createTexture(const TextureCreateInfo& textureInfo) override;

    private:
        IWindow*                mWindow;
        SPtr<detail::Instance>  mInstance;
        UPtr<detail::Surface>   mSurface;
        SPtr<Device>            mDevice;
        UPtr<VulkanSwapchain>   mSwapchain;
    };
}