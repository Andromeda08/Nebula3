#pragma once

#include <vulkan/vulkan.hpp>

#include "Instance.hpp"
#include "VulkanCore.hpp"
#include "Core/Macro.hpp"

namespace RHI
{
    struct DebugContextCreateInfo
    {
        const SPtr<Instance>& instance = nullptr;
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
        SPtr<Instance>              mInstance;
    };
}
