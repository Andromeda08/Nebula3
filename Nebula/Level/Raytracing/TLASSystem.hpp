#pragma once

#include "Level/Instance/InstanceSystem.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class TLASSystem
    {
        struct TLASUpdatePushConstants
        {
            uint32_t size;
        };
    public:
        explicit TLASSystem(const SPtr<RHI::VulkanRHI>& rhi, InstanceSystem* pInstanceSystem);

        void onUpdate(const RHI::FrameData& frameData, const RHI::CommandList* pCommandList) noexcept;

        [[nodiscard]] const SPtr<RHI::AccelerationStructure>& getTLAS() const noexcept
        {
            return mTLAS;
        }

        [[nodiscard]] const SPtr<RHI::Buffer>& getBackingBuffer() const noexcept
        {
            return mBackingBuffer;
        }

        [[nodiscard]] const SPtr<RHI::Descriptor>& getDescriptor() const noexcept;

    private:
        void createInitialEmptyTLAS() noexcept;

        void reallocate(uint32_t instances) noexcept;

        void execute_TLASUpdateInstances(const RHI::CommandList* pCommandList) const noexcept;

        void execute_TLASBuild(const RHI::CommandList* pCommandList) const noexcept;

        SPtr<RHI::VulkanRHI>                mRHI;
        InstanceSystem*                     mInstanceSystem;

        SPtr<RHI::AccelerationStructure>    mTLAS;
        SPtr<RHI::Buffer>                   mBackingBuffer;
        SPtr<RHI::Buffer>                   mBuildScratchBuffer;
        SPtr<RHI::Buffer>                   mInstanceBuffer;

        UPtr<RHI::ComputePipeline>          mUpdatePipeline;
        SPtr<RHI::Descriptor>               mUpdateDescriptor;

        SPtr<RHI::Descriptor>               mDescriptor;

        uint32_t                            mMaxInstances;
    };

}