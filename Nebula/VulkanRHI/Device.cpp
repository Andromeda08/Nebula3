#include "Device.hpp"

namespace RHI
{
    struct VulkanAnyStruct
    {
        vk::StructureType sType;
        const void*       pNext;
    };

    DeviceExtension::DeviceExtension(const char* extensionName, const std::function<void()>& structInitFn)
    : mExtensionName(extensionName)
    , mStructInitFn(structInitFn)
    {
        mIsCoreFeatureStruct = std::string(mExtensionName).contains("VulkanCore");
    }

    void DeviceExtension::preCreateDevice(vk::DeviceCreateInfo& deviceCreateInfo) const
    {
        if (mFeatureStructPtr != nullptr)
        {
            mStructInitFn();
            addToNextChain(deviceCreateInfo, mFeatureStructPtr);
        }
    }

    std::vector<const char*> DeviceExtension::getExtensionNames(const std::vector<UPtr<DeviceExtension>>& extensions) noexcept
    {
        return extensions
            | std::views::filter([](const auto& ext){ return !ext->mIsCoreFeatureStruct; })
            | std::views::transform([](const auto& ext){ return ext->mExtensionName; })
            | std::ranges::to<std::vector<const char*>>();
    }

    void DeviceExtension::addToNextChain(vk::DeviceCreateInfo& deviceCreateInfo, void* featureInfo)
    {
        auto* featureStruct  = static_cast<VulkanAnyStruct*>(featureInfo);
        featureStruct->pNext = deviceCreateInfo.pNext;
        deviceCreateInfo.setPNext(featureInfo);
    }

    // ======================================== //
    // Vulkan Device Extensions                 //
    // ======================================== //
    #pragma region "Define Vulkan Extension Macro"
    #define def_VulkanExt(NAME, STR_EXT_NAME, STRUCT_T, FN)             \
        class Vulkan##NAME : public DeviceExtension {                   \
        public:                                                         \
            Vulkan##NAME() : DeviceExtension(STR_EXT_NAME, FN) {        \
                mFeatureStructPtr = &mFeatureStruct;                    \
            }                                                           \
            ~Vulkan##NAME() override = default;                         \
        private:                                                        \
            STRUCT_T mFeatureStruct;                                    \
        }
    #pragma endregion

    #pragma region "Vulkan Core"

    def_VulkanExt(Core11, "VulkanCore1.1", vk::PhysicalDeviceVulkan11Features, [&]() -> void {
        mFeatureStruct = vk::PhysicalDeviceVulkan11Features();
    });

    def_VulkanExt(Core12, "VulkanCore1.2", vk::PhysicalDeviceVulkan12Features, [&]() -> void {
        mFeatureStruct = vk::PhysicalDeviceVulkan12Features()
            .setBufferDeviceAddress(true)
            .setDescriptorIndexing(true)
            .setScalarBlockLayout(true)
            .setShaderInt8(true)
            .setTimelineSemaphore(true)
            .setHostQueryReset(true)
            .setScalarBlockLayout(true)
            .setDrawIndirectCount(Platform::getDrawIndirectCountSupported());
    });

    def_VulkanExt(Core13, "VulkanCore1.3", vk::PhysicalDeviceVulkan13Features, [&]() -> void {
        mFeatureStruct = vk::PhysicalDeviceVulkan13Features()
            .setMaintenance4(true)
            .setDynamicRendering(true)
            .setSynchronization2(true)
            .setInlineUniformBlock(true);
    });

    def_VulkanExt(Core14, "VulkanCore1.4", vk::PhysicalDeviceVulkan14Features, [&]() -> void {
        mFeatureStruct = vk::PhysicalDeviceVulkan14Features()
            .setHostImageCopy(true)
            .setMaintenance5(true)
            .setMaintenance6(true);
    });

    #pragma endregion

    #pragma region "Extensions with feature structs"

    // VK_KHR_acceleration_structure
    def_VulkanExt(
        AccelerationStructureExt,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
                .setAccelerationStructure(true);
        }
    );

    // VK_KHR_ray_tracing_pipeline
    def_VulkanExt(
        RayTracingPipelineExt,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
                .setRayTracingPipeline(true);
        }
    );

    // VK_KHR_ray_query
    def_VulkanExt(
        RayQueryExt,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        vk::PhysicalDeviceRayQueryFeaturesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceRayQueryFeaturesKHR()
                .setRayQuery(true);
        }
    );

    // VK_EXT_mesh_shader
    def_VulkanExt(
        MeshShaderExt,
        VK_EXT_MESH_SHADER_EXTENSION_NAME,
        vk::PhysicalDeviceMeshShaderFeaturesEXT,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceMeshShaderFeaturesEXT()
                .setMeshShader(true)
                .setTaskShader(true)
                .setMeshShaderQueries(true);
        }
    );

    // VK_EXT_swapchain_maintenance1
    def_VulkanExt(
        SwapchainMaintenance1Ext,
        VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
        vk::PhysicalDeviceSwapchainMaintenance1FeaturesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceSwapchainMaintenance1FeaturesKHR()
                .setSwapchainMaintenance1(true);
        }
    );

    #pragma endregion

    namespace Platform
    {
        std::vector<UPtr<DeviceExtension>> getDeviceExtensions(const RHIFeatureLevel& featureLevel) noexcept
        {
            std::vector<UPtr<DeviceExtension>> extensions;

            if (featureLevel == RHIFeatureLevel::Basic)
            {
                extensions.push_back(std::make_unique<DeviceExtension>(gVulkanPortabilitySubsetExtensionName));
            }

            if (featureLevel >= RHIFeatureLevel::Basic)
            {
                extensions.push_back(std::make_unique<DeviceExtension>(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME));
                extensions.push_back(std::make_unique<DeviceExtension>(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME));
                extensions.push_back(std::make_unique<DeviceExtension>(VK_KHR_SWAPCHAIN_EXTENSION_NAME));
                extensions.push_back(std::make_unique<VulkanCore11>());
                extensions.push_back(std::make_unique<VulkanCore12>());
                extensions.push_back(std::make_unique<VulkanCore13>());
                extensions.push_back(std::make_unique<VulkanCore14>());
                // TODO: Waiting for MoltenVK release 1.4.1
                // extensions.push_back(std::make_unique<DeviceExtension>(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME));
                // extensions.push_back(std::make_unique<VulkanSwapchainMaintenance1Ext>());
            }

            if (featureLevel >= RHIFeatureLevel::Complete)
            {
                extensions.push_back(std::make_unique<VulkanAccelerationStructureExt>());
                extensions.push_back(std::make_unique<VulkanRayTracingPipelineExt>());
                extensions.push_back(std::make_unique<VulkanRayQueryExt>());
                extensions.push_back(std::make_unique<VulkanMeshShaderExt>());
            }

            return extensions;
        }
    }

    // ======================================== //
    // Vulkan Device                            //
    // ======================================== //
    Device::Device(const DeviceCreateInfo& createInfo)
    : mInstance(createInfo.instance)
    {
        const auto& config = Configuration::getConfig();
        mExtensions     = Platform::getDeviceExtensions(config.rhi.featureLevel);
        mExtensionNames = DeviceExtension::getExtensionNames(mExtensions);

        selectPhysicalDevice();
        createDevice();
        createAllocator();
    }

    Device::~Device()
    {
        waitIdle();
        vmaDestroyAllocator(mAllocator);
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
            return evaluateSupport(candidateExtensions, mExtensionNames);
        });

        assert(candidate != std::end(physicalDevices));

        mPhysicalDevice = *candidate;
        mProperties = mPhysicalDevice.getProperties();
        mDeviceName = std::string(mProperties.deviceName.data());
    }

    void Device::createDevice()
    {
        constexpr float queuePriority = 1.0f;
        std::set<QueueFamily> uniqueQueueFamilies;

        const auto graphicsQueue = findQueueFamily(vk::QueueFlagBits::eGraphics);
        assert(graphicsQueue.has_value());

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

        const auto deviceFeatures = Platform::getDeviceFeatures();
        auto createInfo = vk::DeviceCreateInfo()
            .setEnabledExtensionCount(static_cast<uint32_t>(mExtensionNames.size()))
            .setPpEnabledExtensionNames(mExtensionNames.data())
            .setQueueCreateInfoCount(queueCreateInfos.size())
            .setPQueueCreateInfos(queueCreateInfos.data())
            .setPEnabledFeatures(&deviceFeatures);

        for (const auto& extension : mExtensions)
        {
            extension->preCreateDevice(createInfo);
        }

        mDevice = mPhysicalDevice.createDevice(createInfo);

        // mGraphicsQueue = {
        //     .queue       = mDevice.getQueue(graphicsQueue->familyIndex, 0),
        //     .familyIndex = graphicsQueue->familyIndex,
        //     .queueIndex  = 0,
        //     .queueType   = RHIQueueType::Graphics,
        // };
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
             auto&& [familyIndex, properties] : std::views::enumerate(queueFamilies))
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
