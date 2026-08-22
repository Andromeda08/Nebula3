#include "Device.hpp"

#include <map>
#include <vulkan/vk_enum_string_helper.h>

namespace sunflower::rhi
{
    namespace detail
    {
        namespace
        {
            struct QueueFamilyResult
            {
                vk::QueueFamilyProperties properties;
                QueueType                 type;
                uint32_t                  familyIndex;

                [[nodiscard]] DeviceQueue makeDeviceQueue(const vk::Device& device) const
                {
                    const auto queueInfo = vk::DeviceQueueInfo2()
                        .setQueueFamilyIndex(familyIndex)
                        .setQueueIndex(0);
                    return {
                        .queue       = device.getQueue2(queueInfo),
                        .familyIndex = familyIndex,
                        .queueIndex  = 0,
                        .queueType   = type,
                        .properties  = properties,
                    };
                }
            };

            struct QueueFamilySearchParams
            {
                vk::PhysicalDevice  physicalDevice  = VK_NULL_HANDLE;
                QueueType           type            = QueueType::Graphics;
            };
        }

        [[nodiscard]] static Option<QueueFamilyResult> findQueueFamily(const QueueFamilySearchParams& params)
        {
            const auto& [ physicalDevice, type ] = params;

            vk::QueueFlags requiredFlags = {};
            vk::QueueFlags excludedFlags = {};

            using enum vk::QueueFlagBits;
            switch (type)
            {
                case QueueType::Compute:
                {
                    requiredFlags = eCompute;
                    excludedFlags = eGraphics;
                    break;
                }
                case QueueType::Graphics:
                {
                    requiredFlags = eGraphics;
                    break;
                }
                case QueueType::Transfer:
                {
                    requiredFlags = eTransfer;
                    break;
                }
            }

            const std::vector queueFamilies = physicalDevice.getQueueFamilyProperties2();
            for (auto&& [familyIndex, familyProps2] : enumerate(queueFamilies))
            {
                const auto& properties = familyProps2.queueFamilyProperties;
                if ((properties.queueCount > 0)
                    &&  (properties.queueFlags & requiredFlags)
                    && !(properties.queueFlags & excludedFlags))
                {
                    return QueueFamilyResult {
                        .properties  = properties,
                        .type        = type,
                        .familyIndex = static_cast<uint32_t>(familyIndex),
                    };
                }
            }
            return std::nullopt;
        }
    }

    Device::Device(const DeviceCreateInfo& createInfo)
    : mInstance(createInfo.instance)
    {
        /* Extension configuration */ {
            mExtensionLibrary
                .add<Core11>(Req::Required)
                .add<Core12>(Req::Required)
                .add<Core13>(Req::Required)
                .add<Core14>(Req::Required)
                // Swapchain
                .add(vk::KHRSwapchainExtensionName, Req::Required)
                .add<SwapchainMaintenance1>(Req::Required)
                // General extensions
                .add<DeviceFaultEXT>(Req::Optional)
                // Deferred host ops are required for acceleration structures
                .add(vk::KHRDeferredHostOperationsExtensionName, Req::Optional)
                // Ray tracing
                .add<AccelerationStructureKHR>(Req::Optional)
                .add<RayQuery>(Req::Optional)
                .add<RayTracingPipeline>(Req::Optional)
                .add<RayTracingMaintenance1>(Req::Optional)
                .add<RayTracingLinearSweptSpheresNV>(Req::Optional)
                .add<RayTracingInvocationReorderEXT>(Req::Optional)
                .add<RayTracingPositionFetch>(Req::Optional)
                .add<RayTracingValidation>(Req::Optional)
                // Mesh shader
                .add<MeshShaderEXT>(Req::Optional)
                // Vendor: NVIDIA
                .add(vk::NVXImageViewHandleExtensionName, Req::Optional);
        }

        selectPhysicalDevice();
        createDevice(createInfo.pSurface);
        createAllocator();
    }

    Device::~Device()
    {
        if (mDevice)
        {
            std::ignore = mDevice.waitIdle();
            mDevice.destroy();
        }
    }

    const VmaAllocator& Device::getAllocator() const noexcept
    {
        return mAllocator;
    }

    const vk::Device& Device::getHandle() const noexcept
    {
        return mDevice;
    }

    const vk::PhysicalDevice& Device::getPhysicalDevice() const noexcept
    {
        return mPhysicalDevice;
    }

    void Device::selectPhysicalDevice()
    {
        std::vector<std::string> availableDeviceNames;
        std::map<vk::PhysicalDevice, int32_t> scores;

        for (const auto& physicalDevice : mInstance->getHandle().enumeratePhysicalDevices().value)
        {
            const auto props2 = physicalDevice.getProperties2();
            const auto properties = props2.properties;

            // Extension scoring
            const auto extensionScore = mExtensionLibrary.evaluateDeviceSupport(physicalDevice);

            // Device Type scoring
            int32_t deviceTypeScore = 0;
            switch (properties.deviceType)
            {
                case vk::PhysicalDeviceType::eIntegratedGpu:
                    deviceTypeScore = scoring::sDeviceScore_IsIntegratedGPU;
                    break;
                case vk::PhysicalDeviceType::eDiscreteGpu: {
                    deviceTypeScore = scoring::sDeviceScore_IsDedicatedGPU;
                    break;
                }
                default: {
                    deviceTypeScore = 0;
                    break;
                }
            }

            scores[physicalDevice] = extensionScore + deviceTypeScore;
            availableDeviceNames.push_back(fmt::format("{}[Type={}, Score={}]",
                String(properties.deviceName.data()),
                vk::to_string(properties.deviceType),
                scores[physicalDevice]));
        }

        if (scores.empty() || std::ranges::all_of(scores, [](const auto& dsp){return dsp.second <= 0; }))
        {
            exit("{}", fmt::join(availableDeviceNames, ", "));
        }

        mPhysicalDevice = std::ranges::max_element(scores, [](const auto& a, const auto& b) -> bool {
            return a.second < b.second;
        })->first;

        mExtensionLibrary.postPhysicalDeviceSelection(mPhysicalDevice);
        mActiveExtensionNames = mExtensionLibrary.getActiveExtensionNames();

        mDeviceName = String(mExtensionLibrary.getProperties().deviceName.data());
        mVendor     = toVendorID(mExtensionLibrary.getProperties().vendorID);
    }

    void Device::createDevice(const detail::Surface* pSurface)
    {
        static constexpr float sQueuePriority = 1.0f;
        std::set<uint32_t> uniqueQueueFamilies;

        // A queue with the Graphics is required.
        const auto graphicsQueue = detail::findQueueFamily({
            .physicalDevice = mPhysicalDevice,
            .type           = QueueType::Graphics,
        });
        if (!graphicsQueue.has_value())
        {
            ::sunflower::exit("No Graphics queue was found.");
        }

        // Graphics queue must support presenting.
        const auto [presResult, present] = mPhysicalDevice.getSurfaceSupportKHR(graphicsQueue->familyIndex, pSurface->getHandle());
        if (presResult != vk::Result::eSuccess)
        {
            exit("Surface present support eval failed: {}", vk::to_string(presResult));
        }
        if (!present)
        {
            exit("A Graphics queue was found, but with no presentation support.");
        }

        uniqueQueueFamilies.insert(graphicsQueue->familyIndex);

        // Queue for async compute is optional.
        Option<detail::QueueFamilyResult> computeQueue = std::nullopt;
        if constexpr (!conf::gIsApple)
        {
            computeQueue = detail::findQueueFamily({
                .physicalDevice = mPhysicalDevice,
                .type           = QueueType::Compute,
            });
            if (computeQueue.has_value())
            {
                uniqueQueueFamilies.insert(computeQueue->familyIndex);
            }
        }

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.resize(uniqueQueueFamilies.size());
        for (const auto& [i, familyIndex] : enumerate(uniqueQueueFamilies))
        {
            queueCreateInfos[i] = vk::DeviceQueueCreateInfo()
                .setQueueFamilyIndex(familyIndex)
                .setQueueCount(1)
                .setPQueuePriorities(&sQueuePriority);
        }

        const auto deviceFeatures = mExtensionLibrary.getFeatures();
        auto deviceCreateInfo = vk::DeviceCreateInfo()
            .setEnabledExtensionCount(mActiveExtensionNames.size())
            .setPpEnabledExtensionNames(mActiveExtensionNames.data())
            .setQueueCreateInfos(queueCreateInfos)
            .setPEnabledFeatures(&deviceFeatures);

        mExtensionLibrary.preDeviceCreation(deviceCreateInfo);

        const auto [result, device] = mPhysicalDevice.createDevice(deviceCreateInfo);
        if (result != vk::Result::eSuccess)
        {
            exit("Failed to create Vulkan Device: {}", vk::to_string(result));
        }
        mDevice = device;

        mGraphicsQueue = graphicsQueue->makeDeviceQueue(mDevice);
        mComputeQueue  = computeQueue.transform([&](const auto& q){ return q.makeDeviceQueue(mDevice); });
    }

    void Device::createAllocator()
    {
        const VmaAllocatorCreateInfo createInfo = {
            .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT
                | VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT
                | VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT,
            .physicalDevice = mPhysicalDevice,
            .device = mDevice,
            .instance = mInstance->getHandle(),
            .vulkanApiVersion = vk::ApiVersion14,
        };

        const auto result = vmaCreateAllocator(&createInfo, &mAllocator);
        if (result != VK_SUCCESS)
        {
            exit("Failed to create vulkan memory allocator: {}", string_VkResult(result));
        }
    }
}
