#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "Detail/BufferTraits.hpp"

namespace RHI
{
    struct RHIBufferCreateInfo
    {
        uint64_t        size      = 0;
        BufferType      type      = BufferType::Storage;
        std::string     debugName = "Unknown Buffer";
    };

    struct BufferCreateInfo : public RHIBufferCreateInfo
    {
        SPtr<Device> device = nullptr;
    };

    class Buffer
    {
    public:
        nbl_DISABLE_COPY(Buffer);
        nbl_CTOR_SHARED(Buffer);

        ~Buffer();

        void map(void* ptr) const;

        void unmap() const;

        void setData(const void* pData, uint64_t size, uint64_t offset = 0) const;

        void readBack(void* pData, uint64_t size, uint64_t offset = 0) const;

        const vk::Buffer& getHandle()    const { return mBuffer;              }
        uint64_t          getSize()      const { return mSize;                }
        BufferType        getType()      const { return mBufferType;          }
        uint64_t          getAllocSize() const { return mAllocationInfo.size; }
        uint64_t          getAddress()   const { return mDeviceAddress;       }

    private:
        vk::Buffer          mBuffer;

        VmaAllocation       mAllocation;
        VmaAllocationInfo   mAllocationInfo;
        vk::DeviceAddress   mDeviceAddress;

        SPtr<Device>        mDevice;

        const uint64_t      mSize = 0;
        const BufferType    mBufferType;
        const std::string   mName;
    };
}
