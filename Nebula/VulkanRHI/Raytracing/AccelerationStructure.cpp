#include "AccelerationStructure.hpp"

namespace RHI
{
    AccelerationStructure::AccelerationStructure(const AccelerationStructureCreateInfo& createInfo, const SPtr<Device>& device)
    : mDevice(device)
    , mBackingBuffer(createInfo.backingBuffer)
    , mOffset(createInfo.offset)
    , mSize(createInfo.size)
    , mType(createInfo.type)
    , mLabel(createInfo.label)
    {
        const auto type = mType == AccelerationStructureType::BottomLevel
            ? vk::AccelerationStructureTypeKHR::eBottomLevel
            : vk::AccelerationStructureTypeKHR::eTopLevel;

        const auto asCreateInfo = vk::AccelerationStructureCreateInfoKHR()
            .setBuffer(mBackingBuffer->getHandle())
            .setOffset(mOffset)
            .setSize(mSize)
            .setType(type);
        mHandle = mDevice->getHandle().createAccelerationStructureKHR(asCreateInfo);

        const auto addressInfo = vk::AccelerationStructureDeviceAddressInfoKHR()
            .setAccelerationStructure(mHandle);
        mAddress = mDevice->getHandle().getAccelerationStructureAddressKHR(addressInfo);

        mDevice->nameObject<vk::AccelerationStructureKHR>({ mLabel, mHandle });
    }

    AccelerationStructure::~AccelerationStructure()
    {
        if (mHandle)
        {
            mDevice->getHandle().destroyAccelerationStructureKHR(mHandle);
        }
    }

    const vk::AccelerationStructureKHR& AccelerationStructure::getHandle() const noexcept
    {
        return mHandle;
    }

    vk::DeviceAddress AccelerationStructure::getAddress() const noexcept
    {
        return mAddress;
    }

    uint64_t AccelerationStructure::getOffset() const noexcept
    {
        return mOffset;
    }

    uint64_t AccelerationStructure::getSize() const noexcept
    {
        return mSize;
    }
}
