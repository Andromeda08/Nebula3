#include "Utils.hpp"

#include <unordered_map>
#include "Core/Ranges.hpp"

namespace nbl
{
    CopyBatchBuilder& CopyBatchBuilder::addCopy(RHI::Buffer* pDst, const void* pData, const uint64_t size, const uint64_t dstOffset)
    {
        // TODO: Remove from release build, here for dev testing.
        exitOnAssert(pDst  != nullptr, "Destination Buffer must be valid.");
        exitOnAssert(size  != 0,       "Data size must be greater than 0.");
        exitOnAssert(pData != nullptr, "pData must be a valid pointer.");

        mRegions.push_back({
            .regionStart = mAccumulatedSize,
            .pDstBuffer  = pDst,
            .pData       = pData,
            .size        = size,
            .dstOffset   = dstOffset,
        });

        mAccumulatedSize += size;

        return *this;
    }

    uint64_t CopyBatchBuilder::getStagingBufferSize() const noexcept
    {
        return mAccumulatedSize;
    }

    CopyBatchBuilder& CopyBatchBuilder::setStagingData(const RHI::Buffer* pStagingBuffer)
    {
        for (const auto& region : mRegions)
        {
            pStagingBuffer->setData(region.pData, region.size, region.regionStart);
        }

        return *this;
    }

    CopyBatchBuilder& CopyBatchBuilder::recordCopies(const RHI::Buffer* pStagingBuffer,
        const RHI::CommandList* pCommandList)
    {
        // Group regions by target buffer [TargetBuffer -> List of Vulkan Regions]
        std::unordered_map<RHI::Buffer*, std::vector<vk::BufferCopy2>> regionsPerDst;
        for (const auto& [index, region] : enumerate(mRegions))
        {
            regionsPerDst[region.pDstBuffer].push_back({
                region.regionStart, region.dstOffset, region.size
            });
        }

        // Record copies
        for (const auto& [pDstBuffer, regions] : regionsPerDst)
        {
            const auto copyInfo = vk::CopyBufferInfo2()
                .setSrcBuffer(pStagingBuffer->getHandle())
                .setDstBuffer(pDstBuffer->getHandle())
                .setRegions(regions);
            pCommandList->getHandle().copyBuffer2(copyInfo);
        }

        return *this;
    }
}
