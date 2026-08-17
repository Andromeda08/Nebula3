#pragma once

#include <functional>
#include <set>
#include <rhi/vulkan/VulkanCommon.hpp>

namespace sunflower::rhi
{
    namespace scoring
    {
        // PhysicalDevice scoring values for selection
        inline constexpr int32_t sDeviceScore_MissingRequiredExtension  = -10000000;
        inline constexpr int32_t sDeviceScore_HasRequiredExtension      =  100000;
        inline constexpr int32_t sDeviceScore_HasOptionalExtension      =  10000;
        inline constexpr int32_t sDeviceScore_IsDedicatedGPU            =  1000000;
        inline constexpr int32_t sDeviceScore_IsIntegratedGPU           =  10000;
    }

    /**
     * Feature request mode, missing a feature marked as "required" will abort execution.
     */
    enum class Req
    {
        Disabled,
        Optional,
        Required,
    };

    [[nodiscard]] constexpr const char* toString(const Req e) noexcept
    {
        using enum Req;
        switch (e)
        {
            case Disabled: return "Disabled";
            case Optional: return "Optional";
            case Required: return "Required";
        }
        return "Unknown";
    }

    /**
     * "Extension" class used to represent Vulkan extensions, handle their properties and initialization.
     */
    class Extension
    {
    public:
        sunflower_DisableCopy(Extension);

        Extension(const char* extensionName, Req option);

        Extension(const char* extensionName, Req option, const std::function<void()>& featureInitFn);

        virtual ~Extension() = default;

        void preQueryProperties(vk::PhysicalDeviceProperties2& properties2) const noexcept;

        void preCreateDevice(vk::DeviceCreateInfo& deviceCreateInfo) const noexcept;

        void setSupported() noexcept;

        [[nodiscard]] bool isActive() const noexcept;

        [[nodiscard]] const char* getName() const noexcept;

        [[nodiscard]] Req getRequestType() const noexcept;

        [[nodiscard]] std::string toString(size_t width = 0) const noexcept;

    protected:
        void* mFeatureStructPtr    = nullptr;
        void* mPropertiesStructPtr = nullptr;

    private:
        friend class ExtensionLibrary;

        bool                    mIsCoreFeatureStruct = false;
        Req                     mOption              = Req::Optional;
        bool                    mSupported           = false;
        const char*             mExtensionName       = nullptr;
        std::function<void()>   mFeatureInitFn       = [](){};
    };
}

/**
 * Extension-representing classes.
 * Extensions that only require specifying their name during device creation can be added
 * using "ExtensionLibrary::add(const char*, Req)"; all others use "ExtensionLibrary<T>::add(Req)".
 * Extensions with vk::PhysicalDevice...Features structs can be defined using "def_VulkanExt"; if they also
 * have an associated vk::PhysicalDevice...Properties struct, use "def_VulkanExtProps".
 * (Note: Vulkan Core 1.x are handled as special cases by ExtensionLibrary)
 */
namespace sunflower::rhi
{
    // Extension Utility Macro
    // =============================
    #pragma region "def_VulkanExt && def_VulkanExtProps"
    #ifndef def_VulkanExt
    #define def_VulkanExt(NAME, STR_EXT_NAME, STRUCT_T, FN)     \
        class NAME : public Extension {                         \
        public:                                                 \
            constexpr static const char* sName = STR_EXT_NAME;  \
            explicit NAME(const Req option)                     \
            : Extension(STR_EXT_NAME, option, FN) {             \
                mFeatureStructPtr    = &mFeatureStruct;         \
            }                                                   \
            ~NAME() override = default;                         \
        private:                                                \
            STRUCT_T mFeatureStruct;                            \
        }
    #endif
    #ifndef def_VulkanExtProps
    #define def_VulkanExtProps(NAME, STR_EXT_NAME, STRUCT_T, PROPS_T, FN)   \
        class NAME : public Extension {                                     \
        public:                                                             \
            using PropertiesType = PROPS_T;                                 \
            constexpr static const char* sName = STR_EXT_NAME;              \
            explicit NAME(const Req option)                                 \
            : Extension(STR_EXT_NAME, option, FN) {                         \
                mFeatureStructPtr    = &mFeatureStruct;                     \
                mPropertiesStructPtr = &mProperties;                        \
            }                                                               \
            ~NAME() override = default;                                     \
        private:                                                            \
            STRUCT_T mFeatureStruct;                                        \
            PROPS_T  mProperties;                                           \
        }
    #endif
    #pragma endregion

    // Vulkan Extensions
    // =============================
    #pragma region "Vulkan Core Features"
    def_VulkanExtProps(Core11, "Vulkan Core 1.1",
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan11Properties,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceVulkan11Features()
                .setShaderDrawParameters(true);
        }
    );

    def_VulkanExtProps(Core12, "Vulkan Core 1.2",
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan12Properties,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceVulkan12Features()
                .setShaderSampledImageArrayNonUniformIndexing(true)
                .setBufferDeviceAddress(true)
                .setDescriptorIndexing(true)
                .setScalarBlockLayout(true)
                .setShaderInt8(true)
                .setTimelineSemaphore(true)
                .setHostQueryReset(true)
                .setDrawIndirectCount(!conf::gIsMoltenVk);
        }
    );

    def_VulkanExtProps(Core13, "Vulkan Core 1.3",
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan13Properties,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceVulkan13Features()
                .setShaderDemoteToHelperInvocation(true)
                .setMaintenance4(true)
                .setDynamicRendering(true)
                .setSynchronization2(true)
                .setInlineUniformBlock(true);
        }
    );

    def_VulkanExtProps(Core14, "Vulkan Core 1.4",
        vk::PhysicalDeviceVulkan14Features,
        vk::PhysicalDeviceVulkan14Properties,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceVulkan14Features()
                .setPushDescriptor(true)
                .setHostImageCopy(true)
                .setMaintenance5(true)
                .setMaintenance6(true);
        }
    );
    #pragma endregion

    #pragma region "Extensions"
    // VK_KHR_acceleration_structure
    def_VulkanExtProps(
        AccelerationStructureKHR,
        vk::KHRAccelerationStructureExtensionName,
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
                .setAccelerationStructure(true);
        }
    );

    // VK_KHR_ray_tracing_pipeline
    def_VulkanExtProps(
        RayTracingPipeline,
        vk::KHRRayTracingPipelineExtensionName,
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
                .setRayTracingPipeline(true);
        }
    );

    // VK_KHR_ray_tracing_maintenance1
    def_VulkanExt(
        RayTracingMaintenance1,
        vk::KHRRayTracingMaintenance1ExtensionName,
        vk::PhysicalDeviceRayTracingMaintenance1FeaturesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceRayTracingMaintenance1FeaturesKHR()
                .setRayTracingMaintenance1(true);
        }
    );

    // VK_KHR_ray_query
    def_VulkanExt(
        RayQuery,
        vk::KHRRayQueryExtensionName,
        vk::PhysicalDeviceRayQueryFeaturesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceRayQueryFeaturesKHR()
                .setRayQuery(true);
        }
    );

    // VK_EXT_mesh_shader
    def_VulkanExtProps(
        MeshShaderEXT,
        vk::EXTMeshShaderExtensionName,
        vk::PhysicalDeviceMeshShaderFeaturesEXT,
        vk::PhysicalDeviceMeshShaderPropertiesEXT,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceMeshShaderFeaturesEXT()
                .setMeshShader(true)
                .setTaskShader(true)
                .setMeshShaderQueries(true);
        }
    );

    // VK_EXT_swapchain_maintenance1
    def_VulkanExt(
        SwapchainMaintenance1,
        vk::KHRSwapchainMaintenance1ExtensionName,
        vk::PhysicalDeviceSwapchainMaintenance1FeaturesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceSwapchainMaintenance1FeaturesKHR()
                .setSwapchainMaintenance1(true);
        }
    );

    // VK_NV_ray_tracing_linear_swept_spheres
    def_VulkanExt(
        RayTracingLinearSweptSpheresNV,
        vk::NVRayTracingLinearSweptSpheresExtensionName,
        vk::PhysicalDeviceRayTracingLinearSweptSpheresFeaturesNV,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceRayTracingLinearSweptSpheresFeaturesNV()
                .setSpheres(true)
                .setLinearSweptSpheres(true);
        }
    );

    // VK_EXT_device_fault
    def_VulkanExt(
        DeviceFaultEXT,
        vk::EXTDeviceFaultExtensionName,
        vk::PhysicalDeviceFaultFeaturesEXT,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceFaultFeaturesEXT()
                .setDeviceFault(true);
        }
    );

    // VK_EXT_ray_tracing_invocation_reorder
    def_VulkanExtProps(
        RayTracingInvocationReorderEXT,
        vk::EXTRayTracingInvocationReorderExtensionName,
        vk::PhysicalDeviceRayTracingInvocationReorderFeaturesEXT,
        vk::PhysicalDeviceRayTracingInvocationReorderPropertiesEXT,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceRayTracingInvocationReorderFeaturesEXT()
                .setRayTracingInvocationReorder(true);
        }
    );

    // VK_KHR_ray_tracing_position_fetch
    def_VulkanExt(
        RayTracingPositionFetch,
        vk::KHRRayTracingPositionFetchExtensionName,
        vk::PhysicalDeviceRayTracingPositionFetchFeaturesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceRayTracingPositionFetchFeaturesKHR()
                .setRayTracingPositionFetch(true);
        }
    );

    // VK_NV_ray_tracing_validation
    def_VulkanExt(
        RayTracingValidation,
        vk::NVRayTracingValidationExtensionName,
        vk::PhysicalDeviceRayTracingValidationFeaturesNV,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceRayTracingValidationFeaturesNV()
                .setRayTracingValidation(true);
        }
    );

    def_VulkanExt(
        ShaderImageInt64AtomicsEXT,
        vk::EXTShaderImageAtomicInt64ExtensionName,
        vk::PhysicalDeviceShaderImageAtomicInt64FeaturesEXT,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceShaderImageAtomicInt64FeaturesEXT()
                .setShaderImageInt64Atomics(true);
        }
    );
    #pragma endregion
}

// Don't expose macros outside of this file
#undef def_VulkanExt
#undef def_VulkanExtProps

/**
 * ExtensionLibrary: Management and Utilities
 */
namespace sunflower::rhi
{
    template <class T>
    concept IsExtensionClass = std::derived_from<T, Extension> && requires { T::sName; };

    class ExtensionLibrary
    {
    public:
        ExtensionLibrary();

        // Add & Check Extensions
        // ============================================================
        #pragma region

        /**
         * Add an extension by its name.
         */
        ExtensionLibrary& add(const char* extensionName, Req req);

        /**
         * Add an extension by its representative class.
         */
        template <IsExtensionClass T>
        ExtensionLibrary& add(const Req req)
        {
            if (checkIsExtensionRegistered(T::sName))
            {
                return *this;
            }

            mUniqueExtensionNames.insert(T::sName);
            mDeviceExtensions.push_back(makeUnique<T>(req));

            return *this;
        }

        /**
         * Does the extension list contain the specified extension.
         * @param extensionName
         */
        [[nodiscard]] bool hasExtension(const char* extensionName) const noexcept;

        /**
         * Does the extension list contain the specified extension.
         * @tparam T Extension class
         */
        template <IsExtensionClass T>
        [[nodiscard]] bool hasExtension() const noexcept
        {
            return mUniqueExtensionNames.contains(T::sName);
        }

        /**
         * Check if the specified extension is active.
         * @param extensionName
         */
        [[nodiscard]] bool isActive(const char* extensionName) const
        {
            return contains(mActiveExtensionNames, extensionName);
        }

        /**
         * Check if the specified extension is active.
         * @tparam T Extension class
         */
        template <IsExtensionClass T>
        [[nodiscard]] bool isActive() const
        {
            return contains(mActiveExtensionNames, T::sName);
        }

        #pragma endregion

        // Device Creation & Extension Support
        // ============================================================
        #pragma region

        /**
         * Computes a score for the specified physical device based, for a positive score
         * the given physical device must support the required extensions and feature set.
         * @param physicalDevice
         * @return Final score
         */
        [[nodiscard]] int32_t evaluateDeviceSupport(const vk::PhysicalDevice& physicalDevice) const;

        /**
         * Set supported extensions as supported, find and store active extensions and query properties.
         */
        void postPhysicalDeviceSelection(const vk::PhysicalDevice& physicalDevice);

        /**
         * Currently only used to chain feature structs to DeviceCreateInfo.
         */
        void preDeviceCreation(vk::DeviceCreateInfo& deviceCreateInfo) const;

        #pragma endregion

        // Accessors & Utils
        // =============================

        [[nodiscard]] const vk::PhysicalDeviceProperties& getProperties() const;

        [[nodiscard]] const vk::PhysicalDeviceFeatures& getFeatures() const;

        template <IsExtensionClass T>
        [[nodiscard]] Option<std::reference_wrapper<const typename T::PropertiesType>> getExtensionProperties() const noexcept
        {
            auto it = std::ranges::find_if(mDeviceExtensions, [&](const auto& ext) -> bool { return std::string_view{T::sName} == ext->getName(); });

            if (it == std::end(mDeviceExtensions) || !(*it)->isActive() || ((*it)->mPropertiesStructPtr == nullptr))
            {
                return std::nullopt;
            }

            return std::cref(*static_cast<const T::PropertiesType*>((*it)->mPropertiesStructPtr));
        }

        [[nodiscard]] std::vector<std::string_view> getExtensionNames() const noexcept;

        // Note: only valid after "postPhysicalDeviceSelection" has been called!
        [[nodiscard]] const std::vector<Extension*>& getActiveExtensions() const;

        // Note: only valid after "postPhysicalDeviceSelection" has been called!
        [[nodiscard]] const std::vector<const char*>& getActiveExtensionNames() const;

        /**
         * Logs the extension support report for the physical device that was selected.
         * Note: only valid after "postPhysicalDeviceSelection" has been called!
         */
        [[nodiscard]] std::string toString() const;

    private:
        [[nodiscard]] bool checkIsExtensionRegistered(const char* extensionName) const noexcept;

        vk::PhysicalDeviceFeatures      mFeatures;

        std::vector<UPtr<Extension>>    mDeviceExtensions;
        std::set<std::string_view>      mUniqueExtensionNames;

        // ! Valid after "postPhysicalDeviceSelection()" has been called
        bool                            mPostPhysicalDeviceSelection = false;
        std::vector<const char*>        mActiveExtensionNames;
        std::vector<Extension*>         mActiveExtensions;

        // ! Valid after "postPhysicalDeviceSelection()" has been called
        vk::PhysicalDeviceProperties2   mProperties;
    };
}
