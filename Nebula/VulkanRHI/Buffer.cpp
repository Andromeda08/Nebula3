#include "Buffer.hpp"

#include "Device.hpp"
#include "Image.hpp"

namespace RHI
{
    Buffer::Buffer(const BufferCreateInfo& createInfo)
    : mDevice(createInfo.device)
    , mSize(createInfo.size)
    , mBufferType(createInfo.type)
    , mName(createInfo.debugName)
    {
        auto bufferInfo = vk::BufferCreateInfo()
            .setSize(createInfo.size)
            .setUsage(getBufferUsageFlags(mBufferType, mDevice->getFeatureLevel() >= RHIFeatureLevel::Complete));

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = getBufferMemoryFlags(mBufferType);

        if (mBufferType == BufferType::Staging)
        {
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        }

        const auto* pBufferInfo = reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo);
        auto* pBuffer = reinterpret_cast<VkBuffer*>(&mBuffer);
        const auto result = vmaCreateBuffer(mDevice->getAllocator(), pBufferInfo, &allocInfo, pBuffer, &mAllocation, &mAllocationInfo);
        assert(result == VK_SUCCESS);

        mDevice->nameObject<vk::Buffer>({
            .debugName = mName,
            .handle    = mBuffer,
        });

        const auto addressInfo = vk::BufferDeviceAddressInfo().setBuffer(mBuffer);
        mDeviceAddress = mDevice->getHandle().getBufferAddress(&addressInfo);
    }

    Buffer::~Buffer()
    {
        vmaDestroyBuffer(mDevice->getAllocator(), mBuffer, mAllocation);
    }

    void Buffer::map(void* ptr) const
    {
        assert(isMappableBufferMemory(mBufferType));
        vmaMapMemory(mDevice->getAllocator(), mAllocation, &ptr);
    }

    void Buffer::unmap() const
    {
        assert(isMappableBufferMemory(mBufferType));
        vmaUnmapMemory(mDevice->getAllocator(), mAllocation);
    }

    void Buffer::setData(const void* pData, const uint64_t size, const uint64_t offset) const
    {
        assert(isMappableBufferMemory(mBufferType));
        const auto result = vmaCopyMemoryToAllocation(mDevice->getAllocator(), pData, mAllocation, offset, size);
        assert(result == VK_SUCCESS);
    }

    void Buffer::readBack(void* pData, const uint64_t size, const uint64_t offset) const
    {
        assert(isMappableBufferMemory(mBufferType));
        const auto result = vmaCopyAllocationToMemory(mDevice->getAllocator(), mAllocation, offset, pData, size);
        assert(result == VK_SUCCESS);
    }
}