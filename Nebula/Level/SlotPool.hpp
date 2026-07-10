#pragma once

#include <concepts>
#include <type_traits>
#include <vector>

#include "Core/Types.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

/**
 * Generational Handle
 */
struct Handle
{
    uint32_t index      = std::numeric_limits<uint32_t>::max();
    uint32_t generation = 0;

    [[nodiscard]] bool operator==(const Handle& other) const
    {
        return index == other.index && generation == other.generation;
    }

    [[nodiscard]] bool isNull() const
    {
        return index == std::numeric_limits<uint32_t>::max();
    }
};

/**
 * Guarantee at compile-time that the CPU data type can be converted to its GPU-side type.
 */
template <class CpuType, class GpuType>
concept IsConvertibleToGpu = requires(const CpuType& cpuType)
{
    { cpuType.toGPU() } -> std::same_as<GpuType>;
};

template <class CpuType>
struct CpuView
{
    Handle   handle;
    CpuType* data;
};

/**
 * Fixed-size slot pool with generational handles.
 * @tparam CpuType CPU-side representation that's convertible to the GPU-side type
 * @tparam GpuType GPU-side representation
 */
template <class CpuType, class GpuType>
requires IsConvertibleToGpu<CpuType, GpuType> && std::is_trivially_copyable_v<GpuType>
class Pool
{
    // Invalid slot index
    static constexpr auto INVALID_SLOT = std::numeric_limits<uint32_t>::max();
public:
    explicit Pool(const SPtr<RHI::VulkanRHI>& rhi, const std::string& name, const uint32_t maxCapacity = 1024)
    : mRHI(rhi)
    , mName(name)
    , mCapacity(maxCapacity)
    {
        mCpuData.reserve(mCapacity);
        mGenerations.reserve(mCapacity);
        mSlotToDense.reserve(mCapacity);
        mDenseToSlot.reserve(mCapacity);
        mFreeList.reserve(mCapacity);
        mDirtyDense.reserve(mCapacity);

        mBuffer = mRHI->createBuffer({
            .size  = mCapacity * sizeof(GpuType),
            .type  = RHI::BufferType::Storage,
            .label = fmt::format("Pool_{}_StorageBuffer", name),
        });

        mLastUsedStagingBuffer = mStagingBuffer.size() - 1;
        for (auto i = 0; i < mStagingBuffer.size(); i++)
        {
            mStagingBuffer[i] = mRHI->createBuffer({
                .size  = mCapacity * sizeof(GpuType),
                .type  = RHI::BufferType::Staging,
                .label = fmt::format("Pool_{}_StagingBuffer_{}", name, i),
            });
        }
    }

    [[nodiscard]] Handle acquire(const CpuType& data)
    {
        uint32_t slot;
        if (!mFreeList.empty())
        {
            slot = mFreeList.back();
            mFreeList.pop_back();
        }
        else
        {
            if (mCpuData.size() >= mCapacity)
            {
                exitWithError("[{}] Pool capacity exceeded!", mName);
            }
            slot = static_cast<uint32_t>(mCpuData.size());
            mCpuData.emplace_back();
            mGenerations.push_back(0);
            mSlotToDense.push_back(INVALID_SLOT);
        }

        const auto dense = static_cast<uint32_t>(mDenseToSlot.size());
        mDenseToSlot.push_back(slot);
        mCpuData[slot] = data;
        mSlotToDense[slot] = dense;
        mDirtyDense.push_back(dense);

        return { slot, mGenerations[slot] };
    }

    void update(const Handle& handle, const CpuType& data)
    {
        if (!isValid(handle))
        {
            return;
        }

        const uint32_t slot = handle.index;
        mCpuData[slot] = data;
        mDirtyDense.push_back(mSlotToDense[slot]);
    }

    template <typename F>
    requires std::invocable<F&, CpuType&>
    void modify(const Handle& handle, F&& updateFn)
    {
        if (!isValid(handle))
        {
            return;
        }

        const uint32_t slot = handle.index;
        std::forward<F>(updateFn)(mCpuData[slot]);
        mDirtyDense.push_back(mSlotToDense[slot]);
    }

    void release(const Handle& handle)
    {
        if (!isValid(handle))
        {
            return;
        }

        const uint32_t slot      = handle.index;
        const uint32_t dense     = mSlotToDense[slot];
        const auto     lastDense = static_cast<uint32_t>(mDenseToSlot.size() - 1);

        if (dense != lastDense)
        {
            const uint32_t movedSlot = mDenseToSlot[lastDense];
            mDenseToSlot[dense] = movedSlot;
            mSlotToDense[movedSlot] = dense;
            mDirtyDense.push_back(dense);
        }
        mDenseToSlot.pop_back();

        mSlotToDense[slot] = INVALID_SLOT;
        mGenerations[slot] += 1;
        mFreeList.push_back(slot);
    }

    [[nodiscard]] const CpuType* get(const Handle& handle) const
    {
        if (!isValid(handle))
        {
            return nullptr;
        }
        return &mCpuData[handle.index];
    }

    [[nodiscard]] bool isValid(const Handle& handle) const
    {
        return handle.index != INVALID_SLOT
            && handle.index < mGenerations.size()
            && mGenerations[handle.index] == handle.generation;
    }

    /**
     * Upload data to the GPU
     */
    void flush(const RHI::CommandList* pCommandList, const RHI::BufferUsage postCopyUsage = RHI::BufferUsage::All)
    {
        if (mDirtyDense.empty())
        {
            return;
        }

        pCommandList->beginLabel(fmt::format("Pool_{}_Flush", mName));

        const auto stagingIndex = (mLastUsedStagingBuffer + 1) % mStagingBuffer.size();
        mLastUsedStagingBuffer = stagingIndex;

        // Deduplicate
        std::ranges::sort(mDirtyDense);
        mDirtyDense.erase(std::ranges::unique(mDirtyDense).begin(), std::end(mDirtyDense));

        // Drop entries beyond live count
        const auto liveCount = static_cast<uint32_t>(mDenseToSlot.size());
        while (!mDirtyDense.empty() && mDirtyDense.back() >= liveCount)
        {
            mDirtyDense.pop_back();
        }
        if (mDirtyDense.empty())
        {
            pCommandList->endLabel();
            return;
        }

        // Staging & Copies
        auto* staging = static_cast<GpuType*>(mStagingBuffer[stagingIndex]->map());

        std::vector<vk::BufferCopy2> regions;
        regions.reserve(mDirtyDense.size());

        uint32_t stagedCount = 0;
        size_t   i           = 0;
        while (i < mDirtyDense.size())
        {
            // Combine continuous indices into a single copy region
            const uint32_t start = mDirtyDense[i];
            uint32_t       end   = start;

            while (i + 1 < mDirtyDense.size() && mDirtyDense[i + 1] == end + 1)
            {
                end = mDirtyDense[++i];
            }

            const uint32_t runLength   = end - start + 1;
            const uint32_t stagingBase = stagedCount;

            for (uint32_t j = start; j <= end; j++)
            {
                const uint32_t slot = mDenseToSlot[j];
                staging[stagedCount++] = mCpuData[slot].toGPU();
            }

            regions.push_back(vk::BufferCopy2()
                .setSrcOffset(stagingBase * sizeof(GpuType))
                .setDstOffset(start * sizeof(GpuType))
                .setSize(runLength * sizeof(GpuType)));

            i += 1;
        }

        RHI::Barrier()
            .addBarrier(mBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::TransferDst))
            .insert(pCommandList);

        pCommandList->getHandle().copyBuffer2(vk::CopyBufferInfo2()
            .setSrcBuffer(mStagingBuffer[stagingIndex]->getHandle())
            .setDstBuffer(mBuffer->getHandle())
            .setRegions(regions));

        RHI::Barrier()
            .addBarrier(mBuffer->getBarrier(RHI::BufferUsage::TransferDst, postCopyUsage))
            .insert(pCommandList);

        pCommandList->endLabel();
        mDirtyDense.clear();
    }

    /**
     * Dense index (not slot) is used to access on the GPU.
     */
    [[nodiscard]] uint32_t getGpuIndex(const Handle& handle) const
    {
        if (!isValid(handle))
        {
            exitWithError("Bad handle");
        }
        return mSlotToDense[handle.index];
    }

    [[nodiscard]] const SPtr<RHI::Buffer>& getBuffer() const noexcept
    {
        return mBuffer;
    }

    [[nodiscard]] uint32_t getSize() const noexcept
    {
        return mDenseToSlot.size();
    }

    [[nodiscard]] uint32_t getCapacity() const noexcept
    {
        return mCapacity;
    }

    [[nodiscard]] CpuType* getByDense(const uint32_t denseIndex) noexcept
    {
        return &mCpuData[mDenseToSlot[denseIndex]];
    }

    [[nodiscard]] Handle getHandleFromDense(const uint32_t dense) const
    {
        if (dense >= mDenseToSlot.size())
        {
            return {};
        }

        const uint32_t slot = mDenseToSlot[dense];
        return { slot, mGenerations[slot] };
    }

    template <typename F>
    requires std::invocable<F&, CpuView<CpuType>>
    void forEachView(F&& fn)
    {
        for (uint32_t i = 0; i < mDenseToSlot.size(); i++)
        {
            uint32_t slot = mDenseToSlot[i];

            CpuView<CpuType> view {
                .handle = Handle {slot, mGenerations[slot] },
                .data   = &mCpuData[slot],
            };

            fn(view);
        }
    }

private:
    /**
     * Sparse layout on the CPU, free slots are holes that are reused later.
     * Can be reliably indexed by [handle.index]
     */
    std::vector<CpuType>    mCpuData;
    std::vector<uint32_t>   mGenerations;
    std::vector<uint32_t>   mSlotToDense;   // CPU -> GPU Dense layout
    std::vector<uint32_t>   mDenseToSlot;   // GPU -> CPU Sparse layout
    std::vector<uint32_t>   mFreeList;
    std::vector<uint32_t>   mDirtyDense;

    /**
     * GPU-side
     */
    SPtr<RHI::VulkanRHI>             mRHI;
    SPtr<RHI::Buffer>                mBuffer;

    uint32_t                         mLastUsedStagingBuffer = 0;
    PerFrameArray<SPtr<RHI::Buffer>> mStagingBuffer;

    const std::string       mName;
    const uint32_t          mCapacity = 0;
};
