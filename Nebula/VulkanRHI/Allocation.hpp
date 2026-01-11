#pragma once

#include <vk_mem_alloc.h>

namespace RHI
{
    class Device;

    class Allocation
    {
    public:
        explicit Allocation(const VmaAllocator& allocator);

        void mapMemory(void* pData) const noexcept;

        void unmapMemory() const noexcept;

        VmaAllocation getAllocation() const;

        const VmaAllocationInfo& getAllocationInfo() const;

        void free() const;

    private:
        friend class Device;

        VmaAllocator      mAllocator      = nullptr;
        VmaAllocation     mAllocation     = nullptr;
        VmaAllocationInfo mAllocationInfo = {};
    };
}
