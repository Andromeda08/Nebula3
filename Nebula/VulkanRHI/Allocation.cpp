#include "Allocation.hpp"

namespace RHI
{
    Allocation::Allocation(const VmaAllocator& allocator)
    : mAllocator(allocator)
    {
    }

    void Allocation::mapMemory(void* pData) const noexcept
    {
        vmaMapMemory(mAllocator, mAllocation, &pData);
    }

    void Allocation::unmapMemory() const noexcept
    {
        vmaUnmapMemory(mAllocator, mAllocation);
    }

    VmaAllocation Allocation::getAllocation() const
    {
        return mAllocation;
    }

    const VmaAllocationInfo& Allocation::getAllocationInfo() const
    {
        return mAllocationInfo;
    }

    void Allocation::free() const
    {
        vmaFreeMemory(mAllocator, mAllocation);
    }
}
