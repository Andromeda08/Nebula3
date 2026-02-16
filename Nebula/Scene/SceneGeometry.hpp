#pragma once

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
    void uploadQueuedData() noexcept;

    SPtr<RHI::VulkanRHI>        mRHI;
};
