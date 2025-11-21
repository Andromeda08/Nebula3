#include "DebugContext.hpp"

namespace RHI
{
    DebugContext::DebugContext(const DebugContextCreateInfo& createInfo)
    : mInstance(createInfo.instance)
    {
        using S = vk::DebugUtilsMessageSeverityFlagBitsEXT;
        constexpr auto severity = S::eInfo | S::eWarning | S::eError | S::eVerbose;

        using T = vk::DebugUtilsMessageTypeFlagBitsEXT;
        constexpr auto type = T::eGeneral | T::ePerformance | T::eValidation;

        constexpr auto messengerCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(severity)
            .setMessageType(type)
            .setPfnUserCallback(debugMessageCallback);

        mMessenger = mInstance->getHandle().createDebugUtilsMessengerEXT(messengerCreateInfo);
    }

    DebugContext::~DebugContext()
    {
        mInstance->getHandle().destroyDebugUtilsMessengerEXT(mMessenger);
    }

    vk::Bool32 DebugContext::debugMessageCallback(
        const vk::DebugUtilsMessageSeverityFlagBitsEXT  severity,
        const vk::DebugUtilsMessageTypeFlagsEXT         type,
        const vk::DebugUtilsMessengerCallbackDataEXT*   p_data,
        void*                                           p_user)
    {
        if (!p_data) return vk::False;
        if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        {
            const auto msg = std::string(p_data->pMessage);
            std::println("[VulkanValidation] {}", p_data->pMessage);
        }
        return vk::False;
    }
}
