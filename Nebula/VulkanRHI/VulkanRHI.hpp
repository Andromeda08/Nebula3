#pragma once

#include <vulkan/vulkan.hpp>

#include "Buffer.hpp"
#include "DebugContext.hpp"
#include "Device.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Instance.hpp"
#include "IWindow.hpp"
#include "Swapchain.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"

namespace RHI
{
    struct VulkanRHICreateInfo
    {
        SPtr<IWindow> pWindow = nullptr;
    };

    class VulkanRHI
    {
    public:
        nbl_DISABLE_COPY(VulkanRHI);
        nbl_CTOR(VulkanRHI);

        SPtr<Buffer> createBuffer(const RHIBufferCreateInfo& createInfo) const;
        SPtr<Image>  createImage(const RHIImageCreateInfo& createInfo) const;

    private:
        SPtr<IWindow>       mWindow;
        RHIFeatureLevel     mFeatureLevel;

        SPtr<Instance>      mInstance;
        UPtr<DebugContext>  mDebugContext;
        SPtr<Device>        mDevice;
        UPtr<Swapchain>     mSwapchain;

        UPtr<FrameSync>     mFrameSync;
    };
}