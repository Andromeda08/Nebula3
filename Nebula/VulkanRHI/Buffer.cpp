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
    , mProperties(BufferProperties { createInfo.size, createInfo.type, isMappableBufferMemory(createInfo.type) })
    {
        const BufferMemoryAllocationInfo allocInfo = {
            .pHandle    = &mBuffer,
            .bufferType = mProperties.type,
            .bufferInfo = vk::BufferCreateInfo()
                .setSize(createInfo.size)
                .setUsage(getBufferUsageFlags(mProperties.type, gFeatures.rayTracing)),
        };

        const auto allocation = mDevice->allocateBuffer(allocInfo);
        setAllocation(allocation);

        const auto addressInfo = vk::BufferDeviceAddressInfo().setBuffer(mBuffer);
        mDeviceAddress = mDevice->getHandle().getBufferAddress(&addressInfo);

        setLabel(createInfo.label);
        mDevice->nameObject<vk::Buffer>({
            .debugName = mLabel,
            .handle    = mBuffer,
        });
    }

    Buffer::~Buffer()
    {
        vmaDestroyBuffer(mDevice->getAllocator(), mBuffer, mAllocation->getAllocation());
    }

    vk::DescriptorBufferInfo* Buffer::getDescriptorInfo(const std::optional<vk::DeviceSize>& range) noexcept
    {
        if (!mDescriptorBufferInfo.has_value() || range.has_value())
        {
            mDescriptorBufferInfo = { mBuffer, 0, range.value_or(mProperties.size) };
        }
        return &mDescriptorBufferInfo.value();
    }

    void* Buffer::map() const
    {
        nbl_ASSERT_MAPPABLE_MEMORY();
        return mAllocation->getAllocationInfo().pMappedData;
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

    vk::BufferMemoryBarrier2 Buffer::getBarrier(const BufferUsage srcUsage, const BufferUsage dstUsage) const
    {
        const auto [ srcAccess, srcStage ] = getBarrierFlagsForBufferUsage(srcUsage);
        const auto [ dstAccess, dstStage ] = getBarrierFlagsForBufferUsage(dstUsage);
        return vk::BufferMemoryBarrier2()
            .setBuffer(mBuffer)
            .setSize(VK_WHOLE_SIZE)
            .setSrcAccessMask(srcAccess)
            .setDstAccessMask(dstAccess)
            .setSrcStageMask(srcStage)
            .setDstStageMask(dstStage);
    }
}
