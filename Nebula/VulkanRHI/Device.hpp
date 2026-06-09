#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "Allocation.hpp"
#include "RHIFeatures.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "Core/Util.hpp"
#include "Detail/BufferTraits.hpp"
#include "Detail/DeviceExtensions.hpp"

namespace RHI
{
    struct DeviceCreateInfo
    {
        vk::Instance instance;
    };

    template <class T>
    struct NameObjectInfo
    {
        std::string debugName;
        T           handle;
    };

    struct BufferMemoryAllocationInfo
    {
        vk::Buffer*          pHandle;
        BufferType           bufferType;
        vk::BufferCreateInfo bufferInfo;
    };

    struct ImageMemoryAllocationInfo
    {
        vk::Image*          pHandle;
        vk::ImageCreateInfo imageInfo;
    };

    class Device
    {
    public:
        nbl_DISABLE_COPY(Device);
        nbl_CTOR(Device);

        ~Device();

        /**
         * Create Vulkan Buffer and allocate memory.
         * @param allocInfo
         * @return Allocation reference
         */
        [[nodiscard]] SPtr<Allocation> allocateBuffer(const BufferMemoryAllocationInfo& allocInfo) noexcept;

        /**
         * Create Vulkan Image and allocate memory.
         * @param allocInfo
         * @return Allocation reference
         */
        [[nodiscard]] SPtr<Allocation> allocateImage(const ImageMemoryAllocationInfo& allocInfo) noexcept;

        /**
         * Device-level wait idle.
         */
        void waitIdle() const;

        /**
         * Set the debug label of the given Vulkan handle.
         * @tparam T Vulkan handle type
         * @param nameObjectInfo
         */
        template <class T>
        void nameObject(const NameObjectInfo<T>& nameObjectInfo) const;

        [[nodiscard]] const DeviceExtensions& getDeviceExtensions() const noexcept
        {
            return mExtensions;
        }

        [[nodiscard]] vk::PhysicalDevice  getPhysicalDevice() const { return mPhysicalDevice; }
        [[nodiscard]] vk::Device          getHandle()         const { return mDevice; }
        [[nodiscard]] VmaAllocator        getAllocator()      const { return mAllocator; }
        [[nodiscard]] const DeviceQueue&  getGraphicsQueue()  const { return mGraphicsQueue; }
        [[nodiscard]] const std::string&  getDeviceName()     const { return mDeviceName; }

    private:
        void selectPhysicalDevice();

        void createDevice();

        void createAllocator();

        /**
         * Find a device queue by required and excluded flags with the option to exclude specific family indices.
         * @return Queue family info when found
         */
        [[nodiscard]] std::optional<QueueFamilyInfo> findQueueFamily(
            vk::QueueFlags               requiredFlags,
            vk::QueueFlags               excludedFlags    = {},
            const std::set<QueueFamily>& excludedFamilies = {}) const noexcept;

        vk::Instance                        mInstance;
        vk::PhysicalDevice                  mPhysicalDevice;
        vk::PhysicalDeviceProperties        mPhysicalDeviceProperties;

        std::string                         mDeviceName;

        vk::Device                          mDevice;
        DeviceExtensions                    mExtensions = {};
        std::vector<const char*>            mExtensionNames;

        DeviceQueue                         mGraphicsQueue;

        VmaAllocator                        mAllocator {};
        std::vector<SPtr<Allocation>>       mAllocations;
    };

    template<class T>
    void Device::nameObject(const NameObjectInfo<T>& nameObjectInfo) const
    {
        if (!gFeatures.debug)
        {
            return;
        }

        const std::string name = nameObjectInfo.debugName.empty() ? "Unknown" :  nameObjectInfo.debugName;
        const auto nameInfo = vk::DebugUtilsObjectNameInfoEXT()
            .setPObjectName(name.c_str())
            .setObjectHandle(uint64_t(static_cast<T::CType>(nameObjectInfo.handle)))
            .setObjectType(nameObjectInfo.handle.objectType);

        mDevice.setDebugUtilsObjectNameEXT(nameInfo);
    }
}
