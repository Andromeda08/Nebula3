#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "Detail/BufferTraits.hpp"
#include "Detail/Resource.hpp"

namespace RHI
{
    struct RHIBufferCreateInfo
    {
        uint64_t        size = 0;
        BufferType      type = BufferType::Storage;
        std::string     label;
    };

    struct BufferCreateInfo : public RHIBufferCreateInfo
    {
        SPtr<Device> device = nullptr;
    };

    class Buffer : public Resource
    {
    public:
        nbl_DISABLE_COPY(Buffer);
        nbl_CTOR_SHARED(Buffer);

        ~Buffer() override;

        void map(void* ptr) const;

        void unmap() const;

        void setData(const void* pData, uint64_t size, uint64_t offset = 0) const;

        void readBack(void* pData, uint64_t size, uint64_t offset = 0) const;

        const vk::Buffer& getHandle()    const { return mBuffer; }
        uint64_t          getSize()      const { return mProperties.size; }
        BufferType        getType()      const { return mProperties.type; }
        uint64_t          getAllocSize() const { return mAllocation->getAllocationInfo().size; }
        uint64_t          getAddress()   const { return mDeviceAddress; }

    private:
        const BufferProperties  mProperties;
        vk::Buffer              mBuffer;
        vk::DeviceAddress       mDeviceAddress;
    };
}
