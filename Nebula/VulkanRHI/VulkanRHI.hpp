#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "Frame.hpp"
#include "IWindow.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"

namespace RHI
{
    struct VulkanRHICreateInfo
    {
        IWindow* pWindow = nullptr;
    };

    class Instance
    {
    public:
        nbl_DISABLE_COPY(Instance);

        explicit Instance(const VulkanRHICreateInfo& rhiCreateInfo);
        static UPtr<Instance> create(const VulkanRHICreateInfo& rhiCreateInfo) noexcept;

        vk::Instance getHandle() const noexcept
        {
            return mInstance;
        }

    private:
        vk::Instance             mInstance;
        std::vector<const char*> mLayers;
        std::vector<const char*> mExtensions;
    };

    struct DebugContextCreateInfo
    {
        Instance* pInstance = nullptr;
    };

    class DebugContext
    {
    public:
        nbl_DISABLE_COPY(DebugContext);
        nbl_CTOR(DebugContext);

        ~DebugContext();

    private:
        static vk::Bool32 VKAPI_CALL debugMessageCallback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT      severity,
            vk::DebugUtilsMessageTypeFlagsEXT             type,
            const vk::DebugUtilsMessengerCallbackDataEXT* p_data,
            void*                                         p_user);

        vk::DebugUtilsMessengerEXT  mMessenger;
        Instance*                   mInstance;
    };

    class VulkanRHI
    {
    public:
        nbl_DISABLE_COPY(VulkanRHI);
        nbl_CTOR(VulkanRHI);

    private:
        IWindow*            mWindow;
        RHIFeatureLevel     mFeatureLevel;

        UPtr<Device>        mDevice;
        UPtr<DebugContext>  mDebugContext;
        UPtr<Instance>      mInstance;

        UPtr<FrameSync>     mFrameSync;
    };
}