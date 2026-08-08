#include "TLASSystem.hpp"

namespace nbl
{
    struct TLASUpdatePushConstants
    {
        uint64_t tlasInstances;
        uint64_t instances;
        uint32_t size;
    };

    TLASSystem::TLASSystem(const SPtr<RHI::VulkanRHI>& rhi, InstanceSystem* pInstanceSystem)
    : mRHI(rhi)
    , mInstanceSystem(pInstanceSystem)
    , mMaxInstances(0)
    {
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("TLAS_Instance_Update")
            .setPushConstant<TLASUpdatePushConstants>(vk::ShaderStageFlagBits::eCompute)
            .addShader("TLAS_Instances.comp.spv");
        mUpdatePipeline = mRHI->createComputePipeline2(pipelineInfo);

        createInitialEmptyTLAS();

        using enum vk::ShaderStageFlagBits;
        vk::ShaderStageFlags shaderStageFlags = eVertex | eFragment | eCompute | eRaygenKHR | eAnyHitKHR | eClosestHitKHR | eMissKHR | eIntersectionKHR | eCallableKHR;
        if (mRHI->getFeatures().meshShaders)
        {
            shaderStageFlags |= eMeshEXT | eTaskEXT;
        }

        mDescriptor = mRHI->createDescriptor({
            .bindings  = {
                vk::DescriptorSetLayoutBinding()
                    .setBinding(0)
                    .setDescriptorCount(1)
                    .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
                    .setStageFlags(shaderStageFlags),
            },
            .setCount  = RHI::gFramesInFlight,
            .debugName = "TLAS_Descriptor",
        });
    }

    void TLASSystem::onUpdate(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
    {
        const auto instanceCount = mInstanceSystem->getSize();
        if (instanceCount == 0)
        {
            return;
        }
        
        pCommandList->beginLabel("TLAS_Update");

        if (instanceCount > mMaxInstances)
        {
            reallocate(instanceCount);
            spdlog::debug("TLAS reallocated");
        }

        {
            const auto barrier = vk::BufferMemoryBarrier2()
                .setSrcStageMask(vk::PipelineStageFlagBits2::eAllTransfer)
                .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits2::eShaderStorageRead)
                .setBuffer(mInstanceSystem->getBuffer()->getHandle())
                .setSize(VK_WHOLE_SIZE);
            const auto dependencyInfo = vk::DependencyInfo()
                .setBufferMemoryBarriers(barrier);
            pCommandList->getHandle().pipelineBarrier2(dependencyInfo);
        }

        execute_TLASUpdateInstances(pCommandList);

        {
            const auto barrier = vk::BufferMemoryBarrier2()
                .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR)
                .setSrcAccessMask(vk::AccessFlagBits2::eShaderStorageWrite)
                .setDstAccessMask(vk::AccessFlagBits2::eAccelerationStructureReadKHR)
                .setBuffer(mInstanceBuffer->getHandle())
                .setSize(VK_WHOLE_SIZE);
            const auto dependencyInfo = vk::DependencyInfo()
                .setBufferMemoryBarriers(barrier);
            pCommandList->getHandle().pipelineBarrier2(dependencyInfo);
        }

        execute_TLASBuild(pCommandList);

        RHI::Barrier()
            .addBarrier(mInstanceBuffer->getBarrier(RHI::BufferUsage::AS_BuildUpdate, RHI::BufferUsage::AS_Traverse))
            .addBarrier(mBackingBuffer->getBarrier(RHI::BufferUsage::AS_BuildUpdate, RHI::BufferUsage::AS_Traverse))
            .insert(pCommandList);

        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeAccelerationStructure(0, mTLAS);
        mDescriptor->write(frameData.currentFrame, descriptorWrite);

        pCommandList->endLabel();
    }

    const SPtr<RHI::AccelerationStructure>& TLASSystem::getTLAS() const noexcept
    {
        return mTLAS;
    }

    const SPtr<RHI::Buffer>& TLASSystem::getBackingBuffer() const noexcept
    {
        return mBackingBuffer;
    }

    const SPtr<RHI::Descriptor>& TLASSystem::getDescriptor() const noexcept
    {
        return mDescriptor;
    }

    void TLASSystem::createInitialEmptyTLAS() noexcept
    {
        const auto geometry = vk::AccelerationStructureGeometryKHR()
            .setGeometryType(vk::GeometryTypeKHR::eInstances)
            .setGeometry(vk::AccelerationStructureGeometryDataKHR()
                .setInstances(vk::AccelerationStructureGeometryInstancesDataKHR()));
        auto buildInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild
                | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setGeometryCount(1)
            .setPGeometries(&geometry);
        const auto sizesInfo = mRHI->getDevice()->getHandle().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, 0u);
    
        mBackingBuffer = mRHI->createBuffer({
            sizesInfo.accelerationStructureSize, RHI::BufferType::AccelerationStructure,
            "TLAS_Backing_Buffer",
        });
    
        mTLAS = RHI::AccelerationStructure::create({
            .backingBuffer = mBackingBuffer,
            .offset = 0,
            .size = sizesInfo.accelerationStructureSize,
            .type = RHI::AccelerationStructureType::TopLevel,
            .label = "TLAS"
        }, mRHI->getDevice());
    
        const auto scratch = mRHI->createBuffer({
            sizesInfo.buildScratchSize, RHI::BufferType::Storage,
            "TLAS_ScratchBuffer",
        });
    
        buildInfo
            .setDstAccelerationStructure(mTLAS->getHandle())
            .setScratchData(scratch->getAddress());
    
        constexpr auto rangeInfo = vk::AccelerationStructureBuildRangeInfoKHR()
            .setPrimitiveCount(0);
        const auto* pRangeInfo = &rangeInfo;
    
        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void {
           pCommandList->getHandle().buildAccelerationStructuresKHR(1, &buildInfo, &pRangeInfo);
        });
    }
    
    void TLASSystem::reallocate(uint32_t instances) noexcept
    {
        mMaxInstances = instances;
        mInstanceBuffer= mRHI->createBuffer({
            .size  = mMaxInstances * sizeof(vk::AccelerationStructureInstanceKHR),
            .type  = RHI::BufferType::Storage,
            .label = "TLAS_Instances"
        });
    
        #pragma region "TLAS query sizes"

        const auto instanceData = vk::AccelerationStructureGeometryInstancesDataKHR()
            .setData(mInstanceBuffer->getAddress());
        const auto geometryData = vk::AccelerationStructureGeometryDataKHR()
            .setInstances(instanceData);
        const auto geometry = vk::AccelerationStructureGeometryKHR()
            .setGeometryType(vk::GeometryTypeKHR::eInstances)
            .setGeometry(geometryData);
    
        const auto buildInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild
                | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setGeometryCount(1)
            .setPGeometries(&geometry);
    
        const auto sizesInfo = mRHI->getDevice()->getHandle().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, mMaxInstances);

        #pragma endregion
    
        mBackingBuffer = mRHI->createBuffer({
            sizesInfo.accelerationStructureSize, RHI::BufferType::AccelerationStructure,
            "TLAS_Backing_Buffer"
        });
    
        mBuildScratchBuffer = mRHI->createBuffer({
            sizesInfo.buildScratchSize, RHI::BufferType::Storage,
            "TLAS_Scratch_Buffer"
        });
    
        mTLAS = RHI::AccelerationStructure::create({
            .backingBuffer = mBackingBuffer,
            .offset = 0,
            .size = sizesInfo.accelerationStructureSize,
            .type = RHI::AccelerationStructureType::TopLevel,
            .label = "TLAS",
        }, mRHI->getDevice());
    }
    
    void TLASSystem::execute_TLASUpdateInstances(RHI::CommandList* pCommandList) const noexcept
    {
        pCommandList->beginLabel("TLAS_Update_Instances");
    
        const TLASUpdatePushConstants pc = {
            .tlasInstances = mInstanceBuffer->getAddress(),
            .instances = mInstanceSystem->getBuffer()->getAddress(),
            .size = mInstanceSystem->getSize()
        };
    
        pCommandList->bindPipeline(mUpdatePipeline.get());
        pCommandList->pushConstants(&pc);
    
        const auto x = (pc.size + 63) / 64;
        pCommandList->dispatch(x, 1, 1);
    
        pCommandList->endLabel();
    }
    
    void TLASSystem::execute_TLASBuild(RHI::CommandList* pCommandList) const noexcept
    {
        pCommandList->beginLabel("TLAS_Build");
    
        const auto instanceData = vk::AccelerationStructureGeometryInstancesDataKHR()
            .setData(mInstanceBuffer->getAddress());
        const auto geometryData = vk::AccelerationStructureGeometryDataKHR()
            .setInstances(instanceData);
        const auto geometry = vk::AccelerationStructureGeometryKHR()
            .setGeometryType(vk::GeometryTypeKHR::eInstances)
            .setGeometry(geometryData);
    
        const auto buildInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild
                | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setDstAccelerationStructure(mTLAS->getHandle())
            .setScratchData(mBuildScratchBuffer->getAddress())
            .setGeometryCount(1)
            .setPGeometries(&geometry);
    
        const auto rangeInfo = vk::AccelerationStructureBuildRangeInfoKHR()
            .setPrimitiveCount(mInstanceSystem->getSize());
        const auto* pRangeInfo = &rangeInfo;
    
        {
            const auto b1 = vk::BufferMemoryBarrier2()
                .setSrcAccessMask(vk::AccessFlagBits2::eAccelerationStructureWriteKHR | vk::AccessFlagBits2::eAccelerationStructureReadKHR)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR)
                .setDstAccessMask(vk::AccessFlagBits2::eAccelerationStructureWriteKHR)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR)
                .setBuffer(mBuildScratchBuffer->getHandle())
                .setSize(VK_WHOLE_SIZE);
            const auto b2 = vk::BufferMemoryBarrier2()
                .setSrcAccessMask(vk::AccessFlagBits2::eAccelerationStructureWriteKHR)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR)
                .setDstAccessMask(vk::AccessFlagBits2::eAccelerationStructureWriteKHR)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR)
                .setBuffer(mBackingBuffer->getHandle())
                .setSize(VK_WHOLE_SIZE);
            const auto b3 = vk::BufferMemoryBarrier2()
                .setBuffer(mInstanceBuffer->getHandle())
                .setSize(VK_WHOLE_SIZE)
                .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setDstAccessMask(vk::AccessFlagBits2::eAccelerationStructureReadKHR | vk::AccessFlagBits2::eShaderRead)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR | vk::PipelineStageFlagBits2::eAccelerationStructureCopyKHR);
            const std::array barriers = { b1, b2, b3 };
            const auto dependencyInfo = vk::DependencyInfo()
                .setBufferMemoryBarriers(barriers);
            pCommandList->getHandle().pipelineBarrier2(dependencyInfo);
        }
    
        pCommandList->getHandle().buildAccelerationStructuresKHR(1, &buildInfo, &pRangeInfo);
    
        pCommandList->endLabel();
    }
}
