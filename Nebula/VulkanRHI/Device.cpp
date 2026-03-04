#include "Device.hpp"

#include "Texture.hpp"
#include "Core/Ranges.hpp"

namespace RHI
{
    // ======================================== //
    // Vulkan Device                            //
    // ======================================== //
    Device::Device(const DeviceCreateInfo& createInfo)
    : mRHIFeatureLevel(createInfo.featureLevel)
    , mInstance(createInfo.instance)
    {
        mExtensions
            .addPlatformRequiredExtensions()
            .addExtension<Core11>(FeatureOption::Required)
            .addExtension<Core12>(FeatureOption::Required)
            .addExtension<Core13>(FeatureOption::Required)
            .addExtension<Core14>(FeatureOption::Required)
            .addExtension(vk::KHRSwapchainExtensionName, FeatureOption::Required)
            .addExtension(vk::KHRDeferredHostOperationsExtensionName, FeatureOption::Required)
            .addExtension<AccelerationStructureEXT>(FeatureOption::Optional)
            .addExtension<RayTracingPipelineEXT>(FeatureOption::Optional)
            .addExtension<RayQueryEXT>(FeatureOption::Optional)
            .addExtension<MeshShaderEXT>(FeatureOption::Optional);

        const auto& config = Configuration::getConfig();
        mDebugFeatures  = config.rhi.debugFeatures;

        selectPhysicalDeviceV2();
        createDevice();
        createAllocator();

        spdlog::info("GPU ({}) Extension support:\n{}", mDeviceName, mExtensions.toString());
    }

    Device::~Device()
    {
        waitIdle();
        vmaDestroyAllocator(mAllocator);
    }

    SPtr<Allocation> Device::allocateBuffer(const BufferMemoryAllocationInfo& allocInfo) noexcept
    {
        const auto alloc = makeShared<Allocation>(mAllocator);

        VmaAllocationCreateInfo createInfo = {};
        createInfo.usage = allocInfo.bufferType == BufferType::Staging
            ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST
            : VMA_MEMORY_USAGE_AUTO;
        createInfo.flags = getBufferMemoryFlags(allocInfo.bufferType);

        auto* pHandle = reinterpret_cast<VkBuffer*>(allocInfo.pHandle);
        const VkBufferCreateInfo bufferCreateInfo = allocInfo.bufferInfo;
        const auto result = vmaCreateBuffer(mAllocator, &bufferCreateInfo, &createInfo, pHandle,
            &alloc->mAllocation, &alloc->mAllocationInfo);
        nbl_ASSERT(result == VK_SUCCESS, "Failed to create Buffer and allocate memory!");

        mAllocations.push_back(alloc);
        return alloc;
    }

    SPtr<Allocation> Device::allocateImage(const ImageMemoryAllocationInfo& allocInfo) noexcept
    {
        const auto alloc = makeShared<Allocation>(mAllocator);
        VmaAllocationCreateInfo createInfo = {};
        createInfo.usage = VMA_MEMORY_USAGE_AUTO;

        auto* pHandle = reinterpret_cast<VkImage*>(allocInfo.pHandle);
        const VkImageCreateInfo imageCreateInfo = allocInfo.imageInfo;
        const auto result = vmaCreateImage(mAllocator, &imageCreateInfo, &createInfo, pHandle,
            &alloc->mAllocation, &alloc->mAllocationInfo);
        nbl_ASSERT(result == VK_SUCCESS, "Failed to create Image and allocate memory!");

        mAllocations.push_back(alloc);
        return alloc;
    }

    SPtr<Allocation> Device::allocateAliasedImageMemory(const AliasedImageMemoryAllocationInfo& allocInfo) noexcept
    {
        vk::MemoryRequirements finalRequirements = { 0, 0, 0 };
        for (const auto& image : allocInfo.textures)
        {
            vk::MemoryRequirements memoryRequirements;
            mDevice.getImageMemoryRequirements(image->getHandle(), &memoryRequirements);

            finalRequirements.size           = std::max(finalRequirements.size, memoryRequirements.size);
            finalRequirements.alignment      = std::max(finalRequirements.alignment, memoryRequirements.alignment);
            finalRequirements.memoryTypeBits = finalRequirements.memoryTypeBits & memoryRequirements.memoryTypeBits;
        }

        const auto alloc = makeShared<Allocation>(mAllocator);
        alloc->mAliasedUse = true;

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        const VkResult result = vmaAllocateMemory(mAllocator, reinterpret_cast<VkMemoryRequirements*>(&finalRequirements),
            &allocationCreateInfo, &alloc->mAllocation, &alloc->mAllocationInfo);
        nbl_ASSERT(result == VK_SUCCESS, "Failed to allocate memory for aliased Image use!");

        return alloc;
    }

    void Device::waitIdle() const
    {
        mDevice.waitIdle();
    }

    void Device::selectPhysicalDevice()
    {
        const auto physicalDevices = mInstance.enumeratePhysicalDevices();
        const auto candidate = std::ranges::find_if(physicalDevices, [&](const vk::PhysicalDevice& physicalDevice) -> bool {
            const auto candidateExtensions = physicalDevice.enumerateDeviceExtensionProperties();
            return evaluateSupport(candidateExtensions, mExtensions.getExtensionNames());
        });

        exitOnAssert(candidate != std::end(physicalDevices), "No suitable PhysicalDevice was found");

        mPhysicalDevice = *candidate;

        mExtensions.postPhysicalDeviceSelection(mPhysicalDevice);
        mExtensionNames = mExtensions.getActiveExtensionNames();
        mDeviceName = std::string(mExtensions.getProperties().deviceName.data());
    }

    void Device::selectPhysicalDeviceV2()
    {
        std::vector<std::string> availableDeviceNames;
        std::map<vk::PhysicalDevice, int32_t> scores;

        for (const auto& physicalDevice : mInstance.enumeratePhysicalDevices())
        {
            const auto props2 = physicalDevice.getProperties2();
            const auto properties = props2.properties;

            // Extension scoring
            const auto extensionScore = mExtensions.evaluateDeviceSupport(physicalDevice);

            // Device Type scoring
            int32_t deviceTypeScore = 0;
            switch (properties.deviceType)
            {
                case vk::PhysicalDeviceType::eIntegratedGpu:
                    deviceTypeScore = sDeviceScore_IsIntegratedGPU;
                    break;
                case vk::PhysicalDeviceType::eDiscreteGpu: {
                    deviceTypeScore = sDeviceScore_IsDedicatedGPU;
                    break;
                }
                default: {
                    deviceTypeScore = 0;
                    break;
                }
            }

            scores[physicalDevice] = extensionScore + deviceTypeScore;
            availableDeviceNames.push_back(std::format("{}[Type={}, Score={}]",std::string(properties.deviceName.data()), to_string(properties.deviceType), scores[physicalDevice]));
        }

        if (scores.empty() || std::ranges::all_of(scores, [](const auto& dsp){return dsp.second <= 0; }))
        {
            exitWithError("{}", join(availableDeviceNames, ", "));
        }

        mPhysicalDevice = std::ranges::max_element(scores, [](const auto& a, const auto& b) -> bool {
            return a.second < b.second;
        })->first;

        mExtensions.postPhysicalDeviceSelection(mPhysicalDevice);
        mExtensionNames = mExtensions.getActiveExtensionNames();
        mDeviceName = std::string(mExtensions.getProperties().deviceName.data());
    }

    void Device::createDevice()
    {
        constexpr float queuePriority = 1.0f;
        std::set<QueueFamily> uniqueQueueFamilies;

        const auto graphicsQueue = findQueueFamily(vk::QueueFlagBits::eGraphics);
        exitOnAssert(graphicsQueue.has_value(), "No queue with the Graphics bit was found");

        uniqueQueueFamilies.insert(graphicsQueue->familyIndex);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        for (const QueueFamily familyIndex : uniqueQueueFamilies)
        {
            const auto queueCreateInfo = vk::DeviceQueueCreateInfo()
                .setQueueFamilyIndex(familyIndex)
                .setQueueCount(1)
                .setPQueuePriorities(&queuePriority);
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // const auto deviceFeatures = Platform::getDeviceFeatures();
        const auto deviceFeatures = mExtensions.getFeatures();

        auto createInfo = vk::DeviceCreateInfo()
            .setEnabledExtensionCount(static_cast<uint32_t>(mExtensionNames.size()))
            .setPpEnabledExtensionNames(mExtensionNames.data())
            .setQueueCreateInfoCount(queueCreateInfos.size())
            .setPQueueCreateInfos(queueCreateInfos.data())
            .setPEnabledFeatures(&deviceFeatures);

        mExtensions.preDeviceCreation(createInfo);

        mDevice = mPhysicalDevice.createDevice(createInfo);

        mGraphicsQueue = {
            .queue       = mDevice.getQueue(graphicsQueue->familyIndex, 0),
            .familyIndex = graphicsQueue->familyIndex,
            .queueIndex  = 0,
            .queueType   = QueueType::Graphics,
        };
    }

    void Device::createAllocator()
    {
        const VmaAllocatorCreateInfo createInfo = {
            .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
            .physicalDevice = mPhysicalDevice,
            .device = mDevice,
            .instance = mInstance,
            .vulkanApiVersion = VK_API_VERSION_1_4,
        };

        const auto result = vmaCreateAllocator(&createInfo, &mAllocator);
        assert(result == VK_SUCCESS);
    }

    std::optional<QueueFamilyInfo> Device::findQueueFamily(vk::QueueFlags requiredFlags, vk::QueueFlags excludedFlags,
        const std::set<QueueFamily>& excludedFamilies) const noexcept
    {
        for (const std::vector queueFamilies = mPhysicalDevice.getQueueFamilyProperties();
             auto&& [familyIndex, properties] : nbl::enumerate(queueFamilies))
        {
            if ((properties.queueCount > 0)
                && (properties.queueFlags & requiredFlags)
                && !(properties.queueFlags & excludedFlags)
                && !excludedFamilies.contains(familyIndex))
            {
                QueueFamilyInfo familyInfo = { properties, static_cast<uint32_t>(familyIndex) };
                return std::make_optional(familyInfo);
            }
        }
        return std::nullopt;
    }
}
