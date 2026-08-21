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

        [[nodiscard]] const vk::Device& getHandle() const noexcept;

        [[nodiscard]] const VmaAllocator& getAllocator() const noexcept;

        template <class T>
        requires vk::isVulkanHandleType<T>::value
        void setLabel(const T& handle, const Option<String>& label = std::nullopt) const
        {
            if constexpr (conf::gIsDebug)
            {
                if (handle == VK_NULL_HANDLE)
                {
                    return;
                }

                // ReSharper disable once CppFunctionalStyleCast
                const uint64_t objectHandle = uint64_t(static_cast<T::CType>(handle));
                const String   objectName   = label.value_or(fmt::format("{} ({:#x})", vk::to_string(T::objectType), objectHandle));

                const auto objectNameInfo = vk::DebugUtilsObjectNameInfoEXT()
                    .setPObjectName(objectName.c_str())
                    .setObjectHandle(objectHandle)
                    .setObjectType(T::objectType);

                std::ignore = mDevice.setDebugUtilsObjectNameEXT(objectNameInfo);
            }
        }

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
