#pragma once

#include "Level/Instance/InstanceSystem.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class TLASSystem
    {
    public:
        explicit TLASSystem(const SPtr<RHI::VulkanRHI>& rhi, InstanceSystem* pInstanceSystem);

        void onUpdate(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept;

        [[nodiscard]] const SPtr<RHI::AccelerationStructure>& getTLAS() const noexcept;

        [[nodiscard]] const SPtr<RHI::Buffer>& getBackingBuffer() const noexcept;

        [[nodiscard]] const SPtr<RHI::Descriptor>& getDescriptor() const noexcept;

    private:
        void createInitialEmptyTLAS() noexcept;

        void reallocate(uint32_t instances) noexcept;

        void execute_TLASUpdateInstances(RHI::CommandList* pCommandList) const noexcept;

        void execute_TLASBuild(RHI::CommandList* pCommandList) const noexcept;

        SPtr<RHI::VulkanRHI>                mRHI;
        InstanceSystem*                     mInstanceSystem;

        SPtr<RHI::AccelerationStructure>    mTLAS;
        SPtr<RHI::Buffer>                   mBackingBuffer;
        SPtr<RHI::Buffer>                   mBuildScratchBuffer;
        SPtr<RHI::Buffer>                   mInstanceBuffer;

        UPtr<RHI::ComputePipeline2>         mUpdatePipeline;

        SPtr<RHI::Descriptor>               mDescriptor;

        uint32_t                            mMaxInstances;
    };

}