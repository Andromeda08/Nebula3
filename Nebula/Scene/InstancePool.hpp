#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "VulkanRHI/VulkanRHI.hpp"

struct GPUInstanceData
{
    glm::mat4 model;
    glm::vec4 solidColor;
    int32_t   textureIndex;
    int32_t   geometryIndex;
    uint64_t  blasAddress = 0;
    int32_t   normalIndex = -1;
    int32_t   _p0 = 0;
    int32_t   _p1 = 0;
    int32_t   _p2 = 0;
    glm::vec4 min;
    glm::vec4 max;
};

using InstanceIndex = uint32_t;

class InstancePool
{
public:
    using InstanceData = GPUInstanceData;

    explicit InstancePool(const SPtr<RHI::VulkanRHI>& rhi, uint32_t initialCapacity = 256);

    [[nodiscard]] InstanceIndex acquire(const InstanceData& data) noexcept;

    void release(InstanceIndex idx) noexcept;

    // Update instance data at the specified index
    void update(InstanceIndex idx, const InstanceData& data) noexcept;

    // Commit changes to GPU
    void flush(const RHI::CommandList* pCommandList) noexcept;

    [[nodiscard]] const auto& getData() const noexcept
    {
        return mData;
    }

    [[nodiscard]] uint32_t getSize() const noexcept
    {
        return static_cast<uint32_t>(mData.size());
    }

    [[nodiscard]] const SPtr<RHI::Buffer>& getBuffer() const noexcept
    {
        return mInstanceBuffer;
    }

private:
    // Grow capacity by a factor of 2 and copy old data to new buffer
    void resizeBuffer() noexcept;

    SPtr<RHI::VulkanRHI>        mRHI;
    SPtr<RHI::Buffer>           mInstanceBuffer;
    SPtr<RHI::Buffer>           mStagingBuffer;
    uint32_t                    mCapacity;

    std::vector<InstanceIndex>  mUpdateQueue;

    std::vector<InstanceData>   mData;
    std::vector<InstanceIndex>  mFree;
    std::vector<bool>           mDirty;
};
