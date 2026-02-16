#include "SceneGeometry.hpp"

void SceneGeometry::uploadQueuedData() noexcept
{
    uint64_t addVtxSize = 0;
    uint64_t addIdxSize = 0;
    for (const auto& info : mUploadQueue)
    {
        addVtxSize += info.vertexRegion.vertexCount * sizeof(Vertex);
        addIdxSize += info.indexRegion.indexCount   * sizeof(uint32_t);
    }

    // Staging buffer for new data
    // [ addVtxSize | addIdxSize ]
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
            .setDstOffset(oldIdxSize + idxOffset)
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
                .setSize(oldVtxSize);
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

    // Add new committed GeometryInfo structs to meta
    mInfos.append_range(mUploadQueue);

    spdlog::debug("Queued geometry data committed.\n\t- count: {}\n\t- (vertex) {} -> {}\n\t- (index) {} -> {}",
        mUploadQueue.size(), oldVtxSize, oldVtxSize + addVtxSize, oldIdxSize, oldIdxSize + addIdxSize);

    // Clear upload queue
    mUploadQueue.clear();
}
