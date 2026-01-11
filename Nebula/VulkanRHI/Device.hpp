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

namespace RHI
{
    class DeviceExtension
    {
    public:
        explicit DeviceExtension(
            const char*                  extensionName,
            const std::function<void()>& structInitFn = [](){});

        virtual ~DeviceExtension() = default;

        void preCreateDevice(vk::DeviceCreateInfo& deviceCreateInfo) const;

        [[nodiscard]] static std::vector<const char*> getExtensionNames(const std::vector<UPtr<DeviceExtension>>& extensions) noexcept;

    protected:
        void* mFeatureStructPtr = nullptr;

    private:
        static void addToNextChain(vk::DeviceCreateInfo& deviceCreateInfo, void* featureInfo);

        const char*           mExtensionName       = nullptr;
        bool                  mIsCoreFeatureStruct = false;
        std::function<void()> mStructInitFn        = [](){};

    };

    namespace Platform
    {
        [[nodiscard]] constexpr bool getDrawIndirectCountSupported() noexcept
        {
            #ifdef __APPLE__
            return false;
            #endif
            return true;
        }

        [[nodiscard]] std::vector<UPtr<DeviceExtension>> getDeviceExtensions(const RHIFeatureLevel& featureLevel) noexcept;

        [[nodiscard]] inline vk::PhysicalDeviceFeatures getDeviceFeatures() noexcept
        {
            auto features = vk::PhysicalDeviceFeatures()
                .setMultiDrawIndirect(true)
                .setDrawIndirectFirstInstance(true)
                .setFillModeNonSolid(true)
                .setSamplerAnisotropy(true)
                .setSampleRateShading(true)
                .setShaderInt64(true);

            if (Configuration::getConfig().rhi.featureLevel >= RHIFeatureLevel::Complete)
            {
                features.setGeometryShader(true).setTessellationShader(true);
            }

            return features;
        }
    }

    struct DeviceCreateInfo
    {
        RHIFeatureLevel featureLevel;
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

        RHIFeatureLevel getFeatureLevel() const noexcept { return mRHIFeatureLevel; }

    private:
        void selectPhysicalDevice();

        void createDevice();

        void createAllocator();

        [[nodiscard]] std::optional<QueueFamilyInfo> findQueueFamily(
            vk::QueueFlags               requiredFlags,
            vk::QueueFlags               excludedFlags    = {},
            const std::set<QueueFamily>& excludedFamilies = {}) const noexcept;

        RHIFeatureLevel                     mRHIFeatureLevel;
        bool                                mDebugFeatures;
        vk::Instance                        mInstance;
        vk::PhysicalDevice                  mPhysicalDevice;
        vk::PhysicalDeviceProperties        mProperties;
        std::string                         mDeviceName;

        vk::Device                          mDevice;
        std::vector<const char*>            mExtensionNames;
        std::vector<UPtr<DeviceExtension>>  mExtensions;

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
