#pragma once

#include <queue>
#include <vector>

#include "Core/Types.hpp"
#include "Geometry/Geometry.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct VertexBufferRegion
{
    uint64_t firstVertex;
    uint64_t vertexCount;
};

struct IndexBufferRegion
{
    uint64_t firstIndex;
    uint64_t indexCount;
};

struct GeometryInfo
{
    SPtr<Geometry>      geometry;
    VertexBufferRegion  vertexRegion;
    IndexBufferRegion   indexRegion;

    [[nodiscard]] uint64_t getVertexSize() const noexcept
    {
        return geometry->getVertexCount() * sizeof(Vertex);
    }

    [[nodiscard]] uint64_t getIndexSize() const noexcept
    {
        return geometry->getIndexCount() * sizeof(uint32_t);
    }
};

// Geometry Management
// ========================
class SceneGeometry
{
public:
    explicit SceneGeometry(const SPtr<RHI::VulkanRHI>& rhi)
    : mRHI(rhi)
    {
    }

    /**
     * Add a new Geometry type.
     * @param args Geometry subclass ctor params
     */
    template <class T, class... Args>
    requires std::is_base_of_v<Geometry, T>
    SPtr<Geometry> addGeometry(Args&&... args) noexcept
    {
        SPtr<Geometry> geometry = makeShared<T>(std::forward<Args>(args)...);
        mGeometries.push_back(geometry);

        const auto vertexCount = geometry->getVertexCount();
        const auto indexCount = geometry->getIndexCount();

        const GeometryInfo info = {
            .geometry     = geometry,
            .vertexRegion = { mLastVertex, vertexCount },
            .indexRegion  = { mLastIndex, indexCount },
        };

        mLastVertex += vertexCount;
        mLastIndex  += indexCount;

        mUploadQueue.push_back(info);

        return geometry;
    }

    void onUpdate() noexcept
    {
        if (!mUploadQueue.empty())
        {
            uploadQueuedData();
        }
    }

private:
    std::vector<SPtr<Geometry>> mGeometries;

    // GPU resources & meta
    // ========================
    std::vector<GeometryInfo>   mInfos;
    uint64_t                    mLastVertex   = 0;
    SPtr<RHI::Buffer>           mVertexBuffer = nullptr;
    uint64_t                    mLastIndex    = 0;
    SPtr<RHI::Buffer>           mIndexBuffer  = nullptr;

    // Updates
    // ========================
    std::vector<GeometryInfo>   mUploadQueue;

    // Resize buffers and upload queued geometry data
    void uploadQueuedData() noexcept
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

    SPtr<RHI::VulkanRHI>        mRHI;
};
