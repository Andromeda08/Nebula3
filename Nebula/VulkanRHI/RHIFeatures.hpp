#pragma once

#include <string>
#include <vulkan/vulkan.hpp>

namespace RHI
{
    class DeviceExtensions;
}

namespace RHI
{
    enum class AdapterVendor
    {
        AMD,
        Apple,
        Intel,
        NVIDIA,
        Unknown,
    };

    [[nodiscard]] constexpr AdapterVendor getAdapterVendor(const vk::PhysicalDeviceProperties& properties) noexcept
    {
        using enum AdapterVendor;
        if (properties.vendorID == 0x1002) return AMD;
        if (properties.vendorID == 0x106B) return Apple;
        if (properties.vendorID == 0x8086) return Intel;
        if (properties.vendorID == 0x10DE) return NVIDIA;
        return Unknown;
    }

    struct RHIFeatures
    {
        // Feature Sets
        bool rayTracing     = false;    // AS, RT Pipeline, RQ
        bool nvRayTracing   = false;    // NV LSS (RTX 50xx)
        bool geomTess       = false;    // GS, TC, TE Shaders
        bool meshShaders    = false;    // MS, TS Shaders
        bool asyncCompute   = false;
        bool debug          = false;

        // Device Features
        vk::SampleCountFlagBits maxMSAA = vk::SampleCountFlagBits::e1;

        // Physical Device
        std::string     adapterName   = "Unknown";
        AdapterVendor   adapterVendor = AdapterVendor::Unknown;

        void updateFeatureSetsByExtensions(const DeviceExtensions& pExtensions);
    };

    extern RHIFeatures gFeatures;
}
