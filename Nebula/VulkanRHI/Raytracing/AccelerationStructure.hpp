#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include "VulkanRHI/VulkanRHI.hpp"

namespace RHI
{
    enum class AccelerationStructureType
    {
        BottomLevel,
        TopLevel,
    };

    struct AccelerationStructureCreateInfo
    {
        SPtr<Buffer>                backingBuffer;
        uint64_t                    offset;
        uint64_t                    size;
        AccelerationStructureType   type;
        std::string                 label = "";
    };

    class AccelerationStructure
    {
    public:
        rhi_RES_CTOR(AccelerationStructure, Device);

        ~AccelerationStructure();

        [[nodiscard]] const vk::AccelerationStructureKHR& getHandle() const noexcept;

        [[nodiscard]] vk::DeviceAddress getAddress() const noexcept;

        [[nodiscard]] uint64_t getOffset() const noexcept;

        [[nodiscard]] uint64_t getSize() const noexcept;

        [[nodiscard]] AccelerationStructureType getType() const noexcept
        {
            return mType;
        }

    private:
        SPtr<Device>                 mDevice;
        SPtr<Buffer>                 mBackingBuffer;

        vk::AccelerationStructureKHR mHandle;
        vk::DeviceAddress            mAddress;

        uint64_t                     mOffset;
        uint64_t                     mSize;

        AccelerationStructureType    mType;
        std::string                  mLabel;
    };
}