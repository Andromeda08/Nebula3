#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "Core/Util.hpp"

namespace RHI
{
    class Device;

    class Allocation
    {
    public:
        explicit Allocation(const VmaAllocator& allocator);

        void mapMemory(void** pData) const noexcept;

        void unmapMemory() const noexcept;

        VmaAllocation getAllocation() const;

        const VmaAllocationInfo& getAllocationInfo() const;

        void free() const;

        [[nodiscard]] bool allowAliasedUse() const noexcept
        {
            return mAliasedUse;
        }

        void bindAliasedImageMemory(const vk::Image& image) const noexcept
        {
            const auto result = vmaBindImageMemory(mAllocator, mAllocation, image);
            nbl_ASSERT(result == VK_SUCCESS, "Failed to bind image memory!");
        }

    private:
        friend class Device;

        bool              mAliasedUse     = false;
        VmaAllocator      mAllocator      = nullptr;
        VmaAllocation     mAllocation     = nullptr;
        VmaAllocationInfo mAllocationInfo = {};
    };
}
