#pragma once

#include <vk_mem_alloc.h>
#include "Device.hpp"

namespace RHI
{
    class Allocation
    {
    public:
        Allocation(const VmaAllocation alloc, const VmaAllocationInfo& allocationInfo, const SPtr<Device>& device)
        : mAllocation(alloc)
        , mAllocationInfo(allocationInfo)
        , mDevice(device)
        {
        }

        VmaAllocation getAllocation() const
        {
            return mAllocation;
        }

        const VmaAllocationInfo& getAllocationInfo() const
        {
            return mAllocationInfo;
        }

        void free() const
        {
            vmaFreeMemory(mDevice->getAllocator(), mAllocation);
        }

    private:
        VmaAllocation       mAllocation;
        VmaAllocationInfo   mAllocationInfo;
        SPtr<Device>        mDevice;
    };
}
