#include "Buffer.hpp"

#include "Device.hpp"
#include "Image.hpp"
#include "Core/Util.hpp"

#define nbl_ASSERT_MAPPABLE_MEMORY() \
    nbl_ASSERT(isMappableBufferMemory(mProperties.type), "Cannot map memory for BufferType");

namespace RHI
{
    Buffer::Buffer(const BufferCreateInfo& createInfo)
    : Resource(createInfo.device)
    , mProperties(BufferProperties { createInfo.size, createInfo.type })
    {
        const BufferMemoryAllocationInfo allocInfo = {
            .pHandle    = &mBuffer,
            .bufferType = mProperties.type,
            .bufferInfo = vk::BufferCreateInfo()
                .setSize(createInfo.size)
                .setUsage(getBufferUsageFlags(mProperties.type, mDevice->getFeatureLevel() >= RHIFeatureLevel::Complete)),
        };

        const auto allocation = mDevice->allocateBuffer(allocInfo);
        setAllocation(allocation);

        const auto addressInfo = vk::BufferDeviceAddressInfo().setBuffer(mBuffer);
        mDeviceAddress = mDevice->getHandle().getBufferAddress(&addressInfo);

        setLabel(mBuffer, createInfo.label);
    }

    Buffer::~Buffer()
    {
        vmaDestroyBuffer(mDevice->getAllocator(), mBuffer, mAllocation->getAllocation());
    }

    void Buffer::map(void* ptr) const
    {
        nbl_ASSERT_MAPPABLE_MEMORY();
        mAllocation->mapMemory(ptr);
    }

    void Buffer::unmap() const
    {
        nbl_ASSERT_MAPPABLE_MEMORY();
        mAllocation->unmapMemory();
    }

    void Buffer::setData(const void* pData, const uint64_t size, const uint64_t offset) const
    {
        nbl_ASSERT_MAPPABLE_MEMORY();
        const auto result = vmaCopyMemoryToAllocation(mDevice->getAllocator(), pData, mAllocation->getAllocation(), offset, size);
        nbl_ASSERT(result == VK_SUCCESS, "Failed to copy memory to allocation!");
    }

    void Buffer::readBack(void* pData, const uint64_t size, const uint64_t offset) const
    {
        nbl_ASSERT_MAPPABLE_MEMORY();
        const auto result = vmaCopyAllocationToMemory(mDevice->getAllocator(), mAllocation->getAllocation(), offset, pData, size);
        nbl_ASSERT(result == VK_SUCCESS, "Failed to copy from memory to allocation!");
    }
}
