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
        FeatureLevel    featureLevel;
        vk::Instance    instance;
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

    struct AliasedImageMemoryAllocationInfo
    {
        std::vector<SPtr<class Texture>> textures;
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
         * Allocate memory for aliased images.
         * @param allocInfo
         * @return Allocation reference
         */
        [[nodiscard]] SPtr<Allocation> allocateAliasedImageMemory(const AliasedImageMemoryAllocationInfo& allocInfo) noexcept;

        void waitIdle() const;

        template <class T>
        void nameObject(const NameObjectInfo<T>& nameObjectInfo) const;

        const DeviceQueue& getGraphicsQueue() const { return mGraphicsQueue; }

        VmaAllocator getAllocator() const { return mAllocator; }

        vk::PhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
        vk::Device getHandle() const { return mDevice; }
        const std::string& getDeviceName() const noexcept { return mDeviceName; }

        FeatureLevel getFeatureLevel() const noexcept { return mFeatureLevel; }

        [[nodiscard]] const DeviceExtensions& getDeviceExtensions() const noexcept
        {
            return mExtensions;
        }

    private:
        void selectPhysicalDevice();

        void selectPhysicalDeviceV2();

        void createDevice();

        void createAllocator();

        [[nodiscard]] std::optional<QueueFamilyInfo> findQueueFamily(
            vk::QueueFlags               requiredFlags,
            vk::QueueFlags               excludedFlags    = {},
            const std::set<QueueFamily>& excludedFamilies = {}) const noexcept;

        FeatureLevel                        mFeatureLevel;
        bool                                mDebugFeatures;
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
        if (!mDebugFeatures)
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
