#pragma once

#include <rhi/Common.hpp>
#include <rhi/IWindow.hpp>
#include <rhi/vulkan/VulkanCommon.hpp>

namespace sunflower::rhi::detail
{
    struct InstanceCreateInfo
    {
        IWindow*        pWindow         = nullptr;
        Option<String>  applicationName = {};
        Option<String>  engineName      = {};
    };

    /**
     * Vulkan Instance wrapper
     */
    class Instance
    {
    public:
        sunflower_DisableCopy(Instance);
        sunflower_Create(Instance, SPtr);

        ~Instance();

        [[nodiscard]] const vk::Instance& getHandle() const noexcept;

    private:
        vk::Instance                mInstance;
        std::vector<const char*>    mLayers;
        std::vector<const char*>    mExtensions;
    };
}
