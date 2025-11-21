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
            .setUsage(getUsageFlags(mBufferType));

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = getMemoryFlags(mBufferType);

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

    void Buffer::setData(const void* pData, const uint64_t size, const uint64_t offset) const
    {
        const auto result = vmaCopyMemoryToAllocation(mDevice->getAllocator(), pData, mAllocation, offset, size);
        assert(result == VK_SUCCESS);
    }

    void Buffer::readBack(void* pData, const uint64_t size, const uint64_t offset) const
    {
        const auto result = vmaCopyAllocationToMemory(mDevice->getAllocator(), mAllocation, offset, pData, size);
        assert(result == VK_SUCCESS);
    }

    vk::BufferUsageFlags Buffer::getUsageFlags(const BufferType bufferType)
    {
        using enum vk::BufferUsageFlagBits;
        vk::BufferUsageFlags result = eTransferSrc | eTransferDst | eShaderDeviceAddress;

        switch (bufferType)
        {
            case BufferType::Index:{
                result |= eIndexBuffer
                    | eStorageBuffer
                    | eAccelerationStructureBuildInputReadOnlyKHR;
                break;
            }
            case BufferType::Vertex:{
                result |= eVertexBuffer
                    | eStorageBuffer
                    | eAccelerationStructureBuildInputReadOnlyKHR;
                break;
            }
            case BufferType::Indirect:{
                result |= eIndirectBuffer;
                break;
            }
            case BufferType::Storage: {
                result |= eStorageBuffer | eAccelerationStructureBuildInputReadOnlyKHR;
                break;
            }
            case BufferType::Uniform: {
                result |= eUniformBuffer;
                break;
            }
            case BufferType::AccelerationStructure: {
                result |= eAccelerationStructureStorageKHR;
                break;
            }
            case BufferType::ShaderBindingTable: {
                result |= eShaderBindingTableKHR;
                break;
            }
            case BufferType::Staging: {
                break;
            }
        }

        return result;
    }

    int32_t Buffer::getMemoryFlags(const BufferType bufferType)
    {
        switch (bufferType)
        {
            case BufferType::Uniform:
            case BufferType::Staging:
                return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                       | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
                       | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            default:
                return 0;
        }
    }
}