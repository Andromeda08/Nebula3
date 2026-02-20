#include "TLASManager.hpp"

TLASManager::TLASManager(const TLASManagerCreateInfo& createInfo)
: mRHI(createInfo.rhi)
, mInstancePool(createInfo.pInstancePool)
, mMaxInstances(0)
{
    mUpdateDescriptor = mRHI->createDescriptor({
        .bindings  = {
            vk::DescriptorSetLayoutBinding()
                .setBinding(0)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setStageFlags(vk::ShaderStageFlagBits::eCompute),
            vk::DescriptorSetLayoutBinding()
                .setBinding(1)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setStageFlags(vk::ShaderStageFlagBits::eCompute),
        },
        .setCount  = 1,
        .debugName = "TLAS_Instance_Update_Descriptor",
    });

    auto pipelineInfo = RHI::ComputePipelineCreateInfo()
        .setComputeShader(Configuration::getShaderFilePath("TLAS_Instances.comp.spv"))
        .addDescriptorSetLayout(mUpdateDescriptor->getLayout())
        .setPushConstantRange({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(TLASUpdatePushConstants) })
        .setDebugName("TLAS_Instance_Update");
    mUpdatePipeline = mRHI->createComputePipeline(pipelineInfo);

    createInitialEmptyTLAS();
}

void TLASManager::onUpdate(const RHI::CommandList* pCommandList) noexcept
{
    const auto instanceCount = mInstancePool->getSize();
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
            .setBuffer(mInstancePool->getBuffer()->getHandle())
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

    pCommandList->endLabel();
}

void TLASManager::createInitialEmptyTLAS() noexcept
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

void TLASManager::reallocate(uint32_t instances) noexcept
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

    auto writeInfo = RHI::DescriptorWrite()
        .writeStorageBuffer(0, mInstancePool->getBuffer())
        .writeStorageBuffer(1, mInstanceBuffer);
    mUpdateDescriptor->write(0, writeInfo);
}

void TLASManager::execute_TLASUpdateInstances(const RHI::CommandList* pCommandList) const noexcept
{
    pCommandList->beginLabel("TLAS_Update_Instances");

    const TLASUpdatePushConstants pc = { .size = mInstancePool->getSize() };

    mUpdatePipeline->bind(pCommandList->getHandle());
    mUpdatePipeline->bindDescriptorSet(pCommandList->getHandle(), mUpdateDescriptor->getSet(0));
    mUpdatePipeline->pushConstants(pCommandList->getHandle(), &pc);

    const auto x = (pc.size + 63) / 64;
    mUpdatePipeline->dispatch(pCommandList->getHandle(), x, 1, 1);

    pCommandList->endLabel();
}

void TLASManager::execute_TLASBuild(const RHI::CommandList* pCommandList) const noexcept
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
        .setPrimitiveCount(mInstancePool->getSize());
    const auto* pRangeInfo = &rangeInfo;

    pCommandList->getHandle().buildAccelerationStructuresKHR(1, &buildInfo, &pRangeInfo);

    pCommandList->endLabel();
}