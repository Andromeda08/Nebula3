#pragma once

#include "InstancePool.hpp"
#include "SceneGeometry.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct TLASUpdatePushConstants
{
    uint32_t size;
};

struct TLASManagerCreateInfo
{
    SPtr<RHI::VulkanRHI>    rhi;
    InstancePool*           pInstancePool;
};

class TLASManager
{
public:
    nbl_CTOR(TLASManager);

    void onUpdate(const RHI::CommandList* pCommandList) noexcept;

private:
    void createInitialEmptyTLAS() noexcept;

    void reallocate(uint32_t instances) noexcept;

    void execute_TLASUpdateInstances(const RHI::CommandList* pCommandList) const noexcept;

    void execute_TLASBuild(const RHI::CommandList* pCommandList) const noexcept;

    SPtr<RHI::VulkanRHI>                mRHI;
    InstancePool*                       mInstancePool;

    SPtr<RHI::AccelerationStructure>    mTLAS;
    SPtr<RHI::Buffer>                   mBackingBuffer;
    SPtr<RHI::Buffer>                   mBuildScratchBuffer;
    SPtr<RHI::Buffer>                   mInstanceBuffer;

    UPtr<RHI::ComputePipeline>          mUpdatePipeline;
    SPtr<RHI::Descriptor>               mUpdateDescriptor;

    uint32_t                            mMaxInstances;
};
