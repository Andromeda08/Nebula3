#pragma once

#include <vector>

#include "Core/Types.hpp"
#include "Geometry/Geometry.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

/**
 * Aliased type for geometry indices
 */
using GeometryIndex = int32_t;

/**
 * CPU and GPU side Geometry metadata
 */
struct GeometryInfo
{
    // Geometry Index
    int32_t  index      = -1;
    int32_t  _pad0      = -1;

    // VertexBuffer Region
    uint64_t firstVertex = std::numeric_limits<uint64_t>::max();
    uint64_t vertexCount = 0;

    // IndexBuffer Region
    uint64_t firstIndex = std::numeric_limits<uint64_t>::max();
    uint64_t indexCount = 0;

    // Bottom-Level AS Address
    uint64_t blasAddress = 0;

    /**
     * @return Size of geometry vertex data in bytes
     */
    [[nodiscard]] uint64_t getVertexSize() const noexcept
    {
        return vertexCount * sizeof(Vertex);
    }

    /**
     * @return Size of geometry index data in bytes
     */
    [[nodiscard]] uint64_t getIndexSize() const noexcept
    {
        return indexCount * sizeof(uint32_t);
    }

    /**
     * @return Primitive count calculated from index count
     */
    [[nodiscard]] uint32_t getPrimitiveCount() const noexcept
    {
        return static_cast<uint32_t>(indexCount) / 3;
    }
};
static_assert(sizeof(GeometryInfo) % 16 == 0);

/**
 * Group buffers into a single struct for getter method
 */
struct SceneGeometryBuffers
{
    const SPtr<RHI::Buffer>& vertex;
    const SPtr<RHI::Buffer>& index;
    const SPtr<RHI::Buffer>& metadata;
};

/**
 * Non-owning group of all data related to a specific Geometry
 */
struct GeometryView
{
    Geometry*                   geometry;
    const GeometryInfo*         metadata;
    RHI::AccelerationStructure* accelerationStructure;
};

/**
 * Geometry Management
 * - Scene-scoped vertex, index and metadata buffers
 * - Bottom-level Acceleration Structures
 */
class SceneGeometry
{
public:
    explicit SceneGeometry(const SPtr<RHI::VulkanRHI>& rhi);

    [[deprecated("Use commit() instead")]] void onUpdate()
    {
        commit();
    }

    /**
     * Commit the staged geometries to the GPU-side buffers
     * via an immediately executed command list.
     */
    void commit()
    {
        if (!mUploadQueue.empty())
        {
            uploadQueuedGeometries();
        }
    }

    /**
     * Add a new Geometry type, stages a GPU upload for its data.
     * @param args Geometry subclass ctor params
     * @return Index of the Geometry
     */
    template <class T, class... Args>
    requires std::is_base_of_v<Geometry, T>
    GeometryIndex addGeometry(Args&&... args) noexcept
    {
        mGeometries.push_back(makeShared<T>(std::forward<Args>(args)...));
        const auto& geometry = mGeometries.back();

        const GeometryInfo info = {
            .index       = static_cast<int32_t>(mGeometries.size() - 1),
            .firstVertex = mLastVertex,
            .vertexCount = geometry->getVertexCount(),
            .firstIndex  = mLastIndex,
            .indexCount  = geometry->getIndexCount(),
            // BLAS is invalid until it has been built, no GPU commands are executed in this function
            .blasAddress = 0,
        };
        mUploadQueue.push_back(info);

        mLastVertex += info.vertexCount;
        mLastIndex  += info.indexCount;

        return info.index;
    }

    /**
     * @return Grouped getter for Vertex, Index and Metadata buffers
     */
    [[nodiscard]] SceneGeometryBuffers getBuffers() const noexcept
    {
        return {
            .vertex   = mVertexBuffer,
            .index    = mIndexBuffer,
            .metadata = mGeometryInfoBuffer,
        };
    }

    [[nodiscard]] Geometry* getGeometry(const GeometryIndex index) const noexcept
    {
        return mGeometries[index].get();
    }

    [[nodiscard]] uint64_t getBlasAddress(const GeometryIndex index) const noexcept
    {
        return mBottomLevel[index]->getAddress();
    }

    /**
     * @param index Geometry Index
     * @return All data related to a specific Geometry
     */
    [[nodiscard]] GeometryView getGeometryView(const GeometryIndex index) const noexcept
    {
        return {
            .geometry               = mGeometries[index].get(),
            .metadata               = &mInfos[index],
            .accelerationStructure  = mRaytracing ? mBottomLevel[index].get() : nullptr,
        };
    }

    /**
     * @return Number of geometries
     */
    [[nodiscard]] uint32_t getGeometryCount() const noexcept
    {
        return static_cast<uint32_t>(mGeometries.size());
    }

private:
    SPtr<RHI::VulkanRHI>        mRHI;

    // Geometry and Metadata (1-1)
    // ======================================
    std::vector<SPtr<Geometry>> mGeometries;
    std::vector<GeometryInfo>   mInfos;
    uint64_t                    mCommittedCount = 0;

    // GPU-side Resources and Trackers
    // ======================================
    uint64_t                    mLastVertex         = 0;
    SPtr<RHI::Buffer>           mVertexBuffer       = nullptr;
    uint64_t                    mLastIndex          = 0;
    SPtr<RHI::Buffer>           mIndexBuffer        = nullptr;
    SPtr<RHI::Buffer>           mGeometryInfoBuffer = nullptr;

    // RT Acceleration Structures
    // ======================================
    bool                                            mRaytracing = false;
    uint64_t                                        mBLAlignment = 0;
    std::vector<SPtr<RHI::AccelerationStructure>>   mBottomLevel;
    SPtr<RHI::Buffer>                               mBottomLevelData;

    [[nodiscard]] uint64_t alignBLAS(const uint64_t x) const noexcept
    {
        return (x + mBLAlignment - 1) & ~(mBLAlignment - 1);
    }

    // Uploading to the GPU
    // ======================================
    std::vector<GeometryInfo>   mUploadQueue;

    /**
     * 1. Grow the Vertex, Index and Metadata buffers to fit incoming geometries.
     * 2. Upload queued Geometries to the GPU.
     */
    void uploadQueuedGeometries();
};
