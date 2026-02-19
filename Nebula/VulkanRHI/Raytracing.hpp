#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include "Raytracing/RaytracingPipeline.hpp"
#include "Raytracing/ShaderBindingTable.hpp"

namespace RHI
{
    enum class AccelerationStructureType
    {
        BottomLevel,
        TopLevel,
    };

    struct AccelerationStructure
    {
        [[nodiscard]] const vk::AccelerationStructureKHR& getHandle() const noexcept
        {
            return mHandle;
        }

        [[nodiscard]] vk::DeviceAddress getAddress() const noexcept
        {
            return mAddress;
        }

        vk::AccelerationStructureKHR mHandle;
        vk::DeviceAddress            mAddress;
        AccelerationStructureType    mType;
        std::string                  mLabel;
    };
}
