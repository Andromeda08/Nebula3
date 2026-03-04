#include "SceneGeometry.hpp"

#include "Core/Ranges.hpp"

SceneGeometry::SceneGeometry(const SPtr<RHI::VulkanRHI>& rhi): mRHI(rhi)
{
    mRaytracing = mRHI->getFeatureLevel() == RHI::FeatureLevel::Complete;
    if (mRaytracing)
    {
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR asProps;
        vk::PhysicalDeviceProperties2 props2;
        props2.pNext = &asProps;

        mRHI->getDevice()->getPhysicalDevice().getProperties2(&props2);

        mBLAlignment = 256;
    }
}

const SPtr<Geometry>& SceneGeometry::getGeometry(const std::string& name) const noexcept
{
    exitOnAssert(mGeometryLookup.contains(name), "Invalid Geometry name: {}", name);
    return mGeometryLookup.at(name);
}

uint32_t SceneGeometry::getGeometryIndex(const std::string& name) const noexcept
{
    const auto it = std::ranges::find_if(mGeometries, [&name](const auto& geom){ return geom->getName() == name; });
    exitOnAssert(it != std::end(mGeometries), "Invalid Geometry name: {}", name);
    return std::distance(std::begin(mGeometries), it);
}

const SPtr<RHI::AccelerationStructure>& SceneGeometry::getGeometryBLAS(const std::string& name) const noexcept
{
    const auto it = std::ranges::find_if(mGeometries, [&name](const auto& geom){ return geom->getName() == name; });
    exitOnAssert(it != std::end(mGeometries), "Invalid Geometry name: {}", name);
    return mBottomLevel[std::distance(std::begin(mGeometries), it)];
}

void SceneGeometry::onUpdate() noexcept
{
    if (!mUploadQueue.empty())
    {
        uploadQueuedData();
    }
}

void SceneGeometry::uploadQueuedData() noexcept
{
    uint64_t addVtxSize = 0;
    uint64_t addIdxSize = 0;
    for (const auto& info : mUploadQueue)
    {
        addVtxSize += info.vertexRegion.vertexCount * sizeof(Vertex);
        addIdxSize += info.indexRegion.indexCount   * sizeof(uint32_t);
    }

    // Vertex and Index data
    // Staging buffer: [ addVtxSize | addIdxSize ]
    // ================================================
    const auto dataStaging = mRHI->createBuffer({
        .size  = addVtxSize + addIdxSize,
        .type  = RHI::BufferType::Staging,
        .label = "GeometryDataUploadStaging"
    });

    // Set staging data, create copy regions
    const auto oldVtxSize = mVertexBuffer ? mVertexBuffer->getSize() : 0;
    std::vector<vk::BufferCopy2> vtxCopies;

    const auto oldIdxSize = mIndexBuffer ? mIndexBuffer->getSize() : 0;
    std::vector<vk::BufferCopy2> idxCopies;

    // Offsets into new data staging buffer
    uint64_t vtxOffset = 0;
    uint64_t idxOffset = addVtxSize;

    for (const auto& info : mUploadQueue)
    {
        const auto vtxSize = info.getVertexSize();
        dataStaging->setData(info.geometry->getVertices().data(), vtxSize, vtxOffset);

        const auto vtxCopy = vk::BufferCopy2()
            .setSrcOffset(vtxOffset)
            .setDstOffset(oldVtxSize + vtxOffset)
            .setSize(vtxSize);
        vtxCopies.push_back(vtxCopy);

        vtxOffset += vtxSize;

        const auto idxSize = info.getIndexSize();
        dataStaging->setData(info.geometry->getIndices().data(), idxSize, idxOffset);

        const auto idxCopy = vk::BufferCopy2()
            .setSrcOffset(idxOffset)
            .setDstOffset(oldIdxSize + idxOffset - addVtxSize)
            .setSize(idxSize);
        idxCopies.push_back(idxCopy);

        idxOffset += idxSize;
    }

    // Allocate new buffers
    auto newVertexBuffer = mRHI->createBuffer({
        .size  = oldVtxSize + addVtxSize,
        .type  = RHI::BufferType::Vertex,
        .label = "SceneGeometry-VertexBuffer",
    });
    auto newIndexBuffer = mRHI->createBuffer({
        .size  = oldIdxSize + addIdxSize,
        .type  = RHI::BufferType::Index,
        .label = "SceneGeometry-IndexBuffer",
    });

    // Copy data
    mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void
    {
        // Copy old vertex data to front of new buffer
        // ================================================
        if (mVertexBuffer)
        {
            #pragma region
            const auto oldVtxCopy = vk::BufferCopy2()
                .setSrcOffset(0)
                .setDstOffset(0)
                .setSize(oldVtxSize);
            const auto oldVtxCopyInfo = vk::CopyBufferInfo2()
                .setSrcBuffer(mVertexBuffer->getHandle())
                .setDstBuffer(newVertexBuffer->getHandle())
                .setRegions(oldVtxCopy);
            #pragma endregion
            pCommandList->getHandle().copyBuffer2(oldVtxCopyInfo);
        }

        // Copy old index data to front of new buffer
        // ================================================
        if (mIndexBuffer)
        {
            #pragma region
            const auto oldIdxCopy = vk::BufferCopy2()
                .setSrcOffset(0)
                .setDstOffset(0)
                .setSize(oldIdxSize);
            const auto oldIdxCopyInfo = vk::CopyBufferInfo2()
                .setSrcBuffer(mIndexBuffer->getHandle())
                .setDstBuffer(newIndexBuffer->getHandle())
                .setRegions(oldIdxCopy);
            #pragma endregion
            pCommandList->getHandle().copyBuffer2(oldIdxCopyInfo);
        }

        // Copy new data
        // ================================================
        const auto newVtxCopyInfo = vk::CopyBufferInfo2()
            .setSrcBuffer(dataStaging->getHandle())
            .setDstBuffer(newVertexBuffer->getHandle())
            .setRegions(vtxCopies);
        pCommandList->getHandle().copyBuffer2(newVtxCopyInfo);

        const auto newIdxCopyInfo = vk::CopyBufferInfo2()
            .setSrcBuffer(dataStaging->getHandle())
            .setDstBuffer(newIndexBuffer->getHandle())
            .setRegions(idxCopies);
        pCommandList->getHandle().copyBuffer2(newIdxCopyInfo);
    });

    // Replace old buffers
    mVertexBuffer = std::move(newVertexBuffer);
    mIndexBuffer  = std::move(newIndexBuffer);

    // Raytracing
    // ================================================
    if (mRaytracing)
    {
        const auto device = mRHI->getDevice()->getHandle();

        const auto oldBlasCount = mBottomLevel.size();
        const auto addBlasCount = mUploadQueue.size();
        const auto newBlasCount = oldBlasCount + addBlasCount;

        // (New) Geometry Data
        // ================================================
        #pragma region
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR>         buildRangeInfos;
        std::vector<vk::AccelerationStructureGeometryTrianglesDataKHR>  triangleDatas;
        std::vector<vk::AccelerationStructureGeometryKHR>               geometries;
        std::vector<vk::AccelerationStructureBuildGeometryInfoKHR>      buildGeometryInfos;

        buildRangeInfos.reserve(addBlasCount);
        triangleDatas.reserve(addBlasCount);
        geometries.reserve(addBlasCount);
        buildGeometryInfos.reserve(addBlasCount);

        uint64_t vertexBufferOffset = oldVtxSize;
        for (auto&& [queueIdx, geometryInfo] : nbl::enumerate(mUploadQueue))
        {
            const auto buildRangeInfo = vk::AccelerationStructureBuildRangeInfoKHR()
                .setFirstVertex(0)
                .setPrimitiveCount(geometryInfo.getPrimitiveCount())
                .setPrimitiveOffset(geometryInfo.indexRegion.firstIndex * sizeof(uint32_t))
                .setTransformOffset(0);
            buildRangeInfos.push_back(buildRangeInfo);

            const auto triangleData = vk::AccelerationStructureGeometryTrianglesDataKHR()
                .setIndexData(mIndexBuffer->getAddress())
                .setIndexType(vk::IndexType::eUint32)
                .setVertexData(mVertexBuffer->getAddress() + vertexBufferOffset)
                .setVertexFormat(vk::Format::eR32G32B32Sfloat)
                .setVertexStride(sizeof(Vertex))
                .setMaxVertex(geometryInfo.geometry->getVertexCount())
                .setPNext(nullptr);
            triangleDatas.push_back(triangleData);

            const auto geometryData = vk::AccelerationStructureGeometryDataKHR().setTriangles(triangleDatas.back());

            const auto geometry = vk::AccelerationStructureGeometryKHR()
                .setGeometryType(vk::GeometryTypeKHR::eTriangles)
                .setGeometry(geometryData);
            geometries.push_back(geometry);

            const auto buildGeometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
                .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
                .setGeometryCount(1)
                .setGeometries(geometries.back())
                .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
                .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
            buildGeometryInfos.push_back(buildGeometryInfo);

            vertexBufferOffset += geometryInfo.getVertexSize();
        }
        #pragma endregion

        // Resize (or create) backing buffer
        // ================================================
        #pragma region
        // Memory Requirements
        std::vector<vk::AccelerationStructureBuildSizesInfoKHR> buildSizesInfos(addBlasCount);
        for (auto i = 0; i < addBlasCount; i++)
        {
            buildSizesInfos[i] = device.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometryInfos[i], buildRangeInfos[i].primitiveCount);
        }

        const auto oldBufferSize = mBottomLevelData ? mBottomLevelData->getSize() : 0;
        const auto addBufferSize = std::ranges::fold_left(buildSizesInfos, 0,
            [this](uint64_t acc, const auto& bsi){ return acc + alignBLAS(bsi.accelerationStructureSize); });

        auto newBottomLevelData = mRHI->createBuffer({
            .size  = oldBufferSize + addBufferSize,
            .type = RHI::BufferType::AccelerationStructure,
            .label = "SceneGeometry-BLAS-Data",
        });
        #pragma endregion

        const auto staging = mRHI->createBuffer({
            .size  = addBufferSize,
            .type = RHI::BufferType::Storage,
            .label = "SceneGeometry-BLAS-Scratch",
        });

        // Prepare new bottom level build
        // ================================================
        #pragma region
        std::vector<SPtr<RHI::AccelerationStructure>> newBottomLevel(newBlasCount);

        // Copy old BLAS
        if (oldBlasCount > 0)
        {
            for (auto i = 0; i < oldBlasCount; i++)
            {
                newBottomLevel[i] = RHI::AccelerationStructure::create({
                    .backingBuffer = newBottomLevelData,
                    .offset = mBottomLevel[i]->getOffset(),
                    .size = mBottomLevel[i]->getSize(),
                    .type = RHI::AccelerationStructureType::BottomLevel,
                }, mRHI->getDevice());
            }

            mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void {
                for (auto&& [i, blas] : nbl::enumerate(mBottomLevel))
                {
                    const auto copyInfo = vk::CopyAccelerationStructureInfoKHR()
                        .setSrc(blas->getHandle())
                        .setDst(newBottomLevel[i]->getHandle())
                        .setMode(vk::CopyAccelerationStructureModeKHR::eClone);
                    pCommandList->getHandle().copyAccelerationStructureKHR(copyInfo);
                }
            });
        }

        // Offsets for new BLAS
        uint64_t              baseOffset = oldBufferSize;
        std::vector<uint64_t> stagingOffsets;
        vk::DeviceSize        currentOffset = 0;
        for (const auto& buildSizesInfo : buildSizesInfos)
        {
            const auto alignedSize = alignBLAS(buildSizesInfo.accelerationStructureSize);

            stagingOffsets.push_back(currentOffset);
            currentOffset += alignedSize;
        }

        // Create new BLAS handles
        for (auto i = 0; i < addBlasCount; i++)
        {
            newBottomLevel[oldBlasCount + i] = RHI::AccelerationStructure::create({
                .backingBuffer = newBottomLevelData,
                .offset = baseOffset + stagingOffsets[i],
                .size = buildSizesInfos[i].accelerationStructureSize,
                .type = RHI::AccelerationStructureType::BottomLevel,
            }, mRHI->getDevice());
        }
        #pragma endregion

        // Build (new) Bottom Level
        // ================================================
        std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos;
        for (size_t i = 0; i < addBlasCount; i++)
        {
            pBuildRangeInfos.push_back(&buildRangeInfos[i]);

            buildGeometryInfos[i]
                .setDstAccelerationStructure(newBottomLevel[oldBlasCount + i]->getHandle())
                .setScratchData(staging->getAddress() + stagingOffsets[i]);
        }

        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void {
            pCommandList->getHandle().buildAccelerationStructuresKHR(
                pBuildRangeInfos.size(), buildGeometryInfos.data(), pBuildRangeInfos.data());
        });

        mBottomLevel     = std::move(newBottomLevel);
        mBottomLevelData = std::move(newBottomLevelData);

        if (mRaytracing)
        {
            spdlog::debug("BLAS count: {} -> {}", oldBlasCount, newBlasCount);
        }
    }

    // Cleanup
    // ================================================

    // Add new committed GeometryInfo structs to meta
    mInfos.append_range(mUploadQueue);

    spdlog::debug("Queued geometry data committed.\n\t- count: {}\n\t- (vertex) {} -> {}\n\t- (index) {} -> {}",
        mUploadQueue.size(), oldVtxSize, oldVtxSize + addVtxSize, oldIdxSize, oldIdxSize + addIdxSize);

    // Clear upload queue
    mUploadQueue.clear();
}
