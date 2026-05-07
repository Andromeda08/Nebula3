#pragma once

#include <vector>

#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/Commands.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

// Utils.hpp
// A collection of utilities developed during the
// implementation of the hair renderer.
// ============================================================

namespace nbl
{
    /**
     * A utility to build & batch copies from a single staging buffer
     * to multiple target destination buffers. Staging buffer allocation
     * is deferred until all copies are recorded and the required size is known.
     */
    class CopyBatchBuilder
    {
        struct Region
        {
            uint64_t     regionStart = 0;
            RHI::Buffer* pDstBuffer  = nullptr;
            const void*  pData       = nullptr;
            uint64_t     size        = 0;
            uint64_t     dstOffset   = 0;
        };

    public:
        /**
         * Record a copy from the staging buffer to the specified target buffer.
         */
        CopyBatchBuilder& addCopy(RHI::Buffer* pDst, const void* pData, const uint64_t size, const uint64_t dstOffset = 0);

        /**
         * Get the required size for the staging buffer allocation.
         * @note Valid for the copies added before calling this method.
         */
        [[nodiscard]] uint64_t getStagingBufferSize() const noexcept;

        CopyBatchBuilder& setStagingData(const RHI::Buffer* pStagingBuffer);

        CopyBatchBuilder& recordCopies(const RHI::Buffer* pStagingBuffer, const RHI::CommandList* pCommandList);

    private:
        std::vector<Region>  mRegions         = {};
        uint64_t             mAccumulatedSize = 0;
    };
}
