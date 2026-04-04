#include "InstancePool.hpp"

#include "VulkanRHI/Barrier.hpp"

InstancePool::InstancePool(const SPtr<RHI::VulkanRHI>& rhi, const uint32_t initialCapacity)
: mRHI(rhi)
, mCapacity(initialCapacity)
{
    mInstanceBuffer = mRHI->createBuffer({
        .size  = mCapacity * sizeof(InstanceData),
        .type  = RHI::BufferType::Vertex,
        .label = "InstanceBuffer"
    });
    mStagingBuffer = mRHI->createBuffer({
        .size  = mCapacity * sizeof(InstanceData),
        .type  = RHI::BufferType::Staging,
        .label = "InstanceStagingBuffer"
    });
}

InstanceIndex InstancePool::acquire(const InstanceData& data) noexcept
{
    uint32_t index;
    if (!mFree.empty())
    {
        index = mFree.back();
        mFree.pop_back();
        mData[index] = data;
    }
    else
    {
        index = static_cast<uint32_t>(mData.size());
        mData.push_back(data);
        mDirty.push_back(false);
    }
    mDirty[index]  = true;

    if (mData.size() > mCapacity)
    {
        resizeBuffer();
    }

    return index;
}

void InstancePool::release(const InstanceIndex idx) noexcept
{
    mDirty[idx] = true;
    mFree.push_back(idx);
}

void InstancePool::update(const InstanceIndex idx, const InstanceData& data) noexcept
{
    mData[idx]  = data;
    mDirty[idx] = true;
    mUpdateQueue.push_back(idx);
}

void InstancePool::flush(const RHI::CommandList* pCommandList) noexcept
{
    pCommandList->beginLabel("InstancePool_flush");
    std::vector<vk::BufferCopy2> regions;
    std::vector<InstanceData>    staged;
    for (auto i = 0; i < mUpdateQueue.size(); i++)
    {
        auto idx = mUpdateQueue[i];
        if (!mDirty[idx])
        {
            continue;
        }

        staged.push_back(mData[idx]);

        const auto region = vk::BufferCopy2()
            .setSrcOffset((staged.size() - 1) * sizeof(InstanceData))
            .setDstOffset(i * sizeof(InstanceData))
            .setSize(sizeof(InstanceData));
        regions.push_back(region);

        mDirty[idx] = false;
    }

    if (regions.empty())
    {
        return;
    }

    mStagingBuffer->setData(staged.data(), staged.size() * sizeof(InstanceData), 0);

    const auto copyInfo = vk::CopyBufferInfo2()
        .setSrcBuffer(mStagingBuffer->getHandle())
        .setDstBuffer(mInstanceBuffer->getHandle())
        .setRegions(regions);

    RHI::Barrier()
        .addBarrier(mInstanceBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::TransferDst))
        .insert(pCommandList);

    pCommandList->getHandle().copyBuffer2(copyInfo);
    pCommandList->endLabel();

    mUpdateQueue.clear();
}

void InstancePool::resizeBuffer() noexcept
{
    const auto oldCapacity = mCapacity;
    mCapacity *= 2;

    auto newInstanceBuffer = mRHI->createBuffer({
        .size  = mCapacity * sizeof(InstanceData),
        .type  = RHI::BufferType::Vertex,
        .label = "InstanceBuffer"
    });

    // Copy old content
    mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void
    {
        const auto region = vk::BufferCopy2()
            .setSrcOffset(0)
            .setDstOffset(0)
            .setSize(oldCapacity * sizeof(InstanceData));
        const auto copyInfo = vk::CopyBufferInfo2()
            .setSrcBuffer(mInstanceBuffer->getHandle())
            .setDstBuffer(newInstanceBuffer->getHandle())
            .setRegions(region);
        pCommandList->getHandle().copyBuffer2(copyInfo);
    });

    mInstanceBuffer = std::move(newInstanceBuffer);

    mStagingBuffer = mRHI->createBuffer({
        .size  = mCapacity * sizeof(InstanceData),
        .type  = RHI::BufferType::Staging,
        .label = "InstanceStagingBuffer"
    });

    spdlog::warn("InstancePool resized: {} -> {}", oldCapacity, mCapacity);
}
