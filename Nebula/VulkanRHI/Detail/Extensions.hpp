#pragma once

#include <functional>
#include <string>
#include <vulkan/vulkan.hpp>

#include "Core/Util.hpp"
#include "VulkanRHI/Common.hpp"

namespace RHI
{
    enum class FeatureOption
    {
        Disabled = 0,
        Optional = 1,
        Required = 2,
    };

    [[nodiscard]] constexpr std::string_view toString(const FeatureOption e) noexcept
    {
        using enum FeatureOption;
        switch (e)
        {
            case Disabled: return "Disabled";
            case Optional: return "Optional";
            case Required: return "Required";
        }
        return "Unknown";
    }

    class Extension
    {
    public:
        Extension(const char* extensionName, FeatureOption option);

        Extension(const char* extensionName, FeatureOption option, const std::function<void()>& featureInitFn);

        virtual ~Extension() = default;

        void preQueryProperties(vk::PhysicalDeviceProperties2& properties2) const noexcept;

        void preCreateDevice(vk::DeviceCreateInfo& deviceCreateInfo) const noexcept;

        void setSupported() noexcept;

        [[nodiscard]] bool isActive() const noexcept;

        [[nodiscard]] const char* getName() const noexcept;

        [[nodiscard]] FeatureOption getRequestType() const noexcept;

        [[nodiscard]] std::string toString(size_t width = 0) const noexcept;

    protected:
        void* mFeatureStructPtr    = nullptr;
        void* mPropertiesStructPtr = nullptr;

    private:
        friend class DeviceExtensions;

        bool                    mIsCoreFeatureStruct = false;
        FeatureOption           mOption              = FeatureOption::Optional;
        bool                    mSupported           = false;
        const char*             mExtensionName       = nullptr;
        std::function<void()>   mFeatureInitFn       = [](){};
    };

    // Extension Utility Macro
    // =============================
    #pragma region "def_VulkanExt && def_VulkanExtProps"
    #ifndef def_VulkanExt
    #define def_VulkanExt(NAME, STR_EXT_NAME, STRUCT_T, FN)     \
        class NAME : public Extension {                         \
        public:                                                 \
            constexpr static const char* sName = STR_EXT_NAME;  \
            explicit NAME(const FeatureOption option)           \
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
            explicit NAME(const FeatureOption option)                       \
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
    def_VulkanExtProps(Core11, "VulkanCore1.1",
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan11Properties,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceVulkan11Features();
        }
    );

    def_VulkanExtProps(Core12, "VulkanCore1.2",
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan12Properties,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceVulkan12Features()
                .setBufferDeviceAddress(true)
                .setDescriptorIndexing(true)
                .setScalarBlockLayout(true)
                .setShaderInt8(true)
                .setTimelineSemaphore(true)
                .setHostQueryReset(true)
                .setScalarBlockLayout(true)
                .setDrawIndirectCount(!Platform::isApple);
        }
    );

    def_VulkanExtProps(Core13, "VulkanCore1.3",
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan13Properties,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceVulkan13Features()
                .setMaintenance4(true)
                .setDynamicRendering(true)
                .setSynchronization2(true)
                .setInlineUniformBlock(true);
        }
    );

    def_VulkanExtProps(Core14, "VulkanCore1.4",
        vk::PhysicalDeviceVulkan14Features,
        vk::PhysicalDeviceVulkan14Properties,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceVulkan14Features()
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
        SwapchainMaintenance1EXT,
        vk::KHRSwapchainMaintenance1ExtensionName,
        vk::PhysicalDeviceSwapchainMaintenance1FeaturesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceSwapchainMaintenance1FeaturesKHR()
                .setSwapchainMaintenance1(true);
        }
    );

    // VK_EXT_descriptor_heap
    def_VulkanExtProps(
        DescriptorHeapEXT,
        vk::EXTDescriptorHeapExtensionName,
        vk::PhysicalDeviceDescriptorHeapFeaturesEXT,
        vk::PhysicalDeviceDescriptorHeapPropertiesEXT,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceDescriptorHeapFeaturesEXT()
                .setDescriptorHeap(true);
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

    // VK_NV_ray_tracing_linear_swept_spheres
    def_VulkanExt(
        RayTracingPositionFetch,
        vk::KHRRayTracingPositionFetchExtensionName,
        vk::PhysicalDeviceRayTracingPositionFetchFeaturesKHR,
        [&]() -> void {
            mFeatureStruct = vk::PhysicalDeviceRayTracingPositionFetchFeaturesKHR()
                .setRayTracingPositionFetch(true);
        }
    );
    #pragma endregion
}

// Don't expose the macro outside of this file
#undef def_VulkanExt
