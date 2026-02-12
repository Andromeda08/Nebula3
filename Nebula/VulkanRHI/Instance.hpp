#pragma once

#include "../RHI/IWindow.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"

namespace RHI
{
    struct InstanceCreateInfo
    {
        IWindow* pWindow = nullptr;
    };

    class Instance
    {
    public:
        nbl_DISABLE_COPY(Instance);
        nbl_CTOR_SHARED(Instance);

        vk::Instance getHandle() const noexcept
        {
            return mInstance;
        }

    private:
        vk::Instance             mInstance;
        std::vector<const char*> mLayers;
        std::vector<const char*> mExtensions;
    };
}