#pragma once

#include <rhi/Common.hpp>
#include <rhi/vulkan/VulkanCommon.hpp>
#include <rhi/vulkan/detail/ExtensionDef.hpp>
#include <rhi/vulkan/detail/Instance.hpp>
#include <rhi/vulkan/detail/Surface.hpp>

namespace sunflower::rhi
{
    struct DeviceCreateInfo
    {
        SPtr<detail::Instance> instance;
        detail::Surface*       pSurface;
    };

    class Device final
    {
    public:
        sunflower_DisableCopy(Device);
        sunflower_Create(Device, SPtr);

        ~Device();

    private:
        void selectPhysicalDevice();
        void createDevice(const detail::Surface* pSurface);
        void createAllocator();

        SPtr<detail::Instance>      mInstance;
        ExtensionLibrary            mExtensionLibrary;

        vk::PhysicalDevice          mPhysicalDevice;
        vk::Device                  mDevice;
        VmaAllocator                mAllocator = nullptr;

        std::vector<const char*>    mActiveExtensionNames;
        String                      mDeviceName;
        VendorID                    mVendor = VendorID::Other;

        DeviceQueue                 mGraphicsQueue;
        Option<DeviceQueue>         mComputeQueue;

    };
}
