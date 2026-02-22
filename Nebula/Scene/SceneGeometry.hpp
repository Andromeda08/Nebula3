#pragma once

#include <map>
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

    [[nodiscard]] uint32_t getPrimitiveCount() const noexcept
    {
        return static_cast<uint32_t>(indexRegion.indexCount) / 3;
    }
};

// Geometry Management
// ========================
class SceneGeometry
{
public:
    explicit SceneGeometry(const SPtr<RHI::VulkanRHI>& rhi);

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
        mGeometryLookup.insert_or_assign(geometry->getName(), geometry);

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

    [[nodiscard]] const SPtr<Geometry>& getGeometry(const std::string& name) const noexcept;

    [[nodiscard]] uint32_t getGeometryIndex(const std::string& name) const noexcept;

    [[nodiscard]] const GeometryInfo& getGeometryInfo(const std::string& name) const noexcept
    {
        return mInfos[getGeometryIndex(name)];
    }

    [[nodiscard]] const SPtr<RHI::AccelerationStructure>& getGeometryBLAS(const std::string& name) const noexcept;

    [[nodiscard]] const SPtr<RHI::Buffer>& getVertexBuffer() const noexcept
    {
        return mVertexBuffer;
    }

    [[nodiscard]] const SPtr<RHI::Buffer>& getIndexBuffer() const noexcept
    {
        return mIndexBuffer;
    }

    [[nodiscard]] uint32_t getCount() const noexcept
    {
        return static_cast<uint32_t>(mGeometries.size());
    }

    void onUpdate() noexcept;

private:
    std::map<std::string, SPtr<Geometry>> mGeometryLookup;
    std::vector<SPtr<Geometry>>           mGeometries;

    // GPU resources & meta
    // ========================
    std::vector<GeometryInfo>   mInfos;
    uint64_t                    mLastVertex   = 0;
    SPtr<RHI::Buffer>           mVertexBuffer = nullptr;
    uint64_t                    mLastIndex    = 0;
    SPtr<RHI::Buffer>           mIndexBuffer  = nullptr;

    // Raytracing
    // ========================
    bool                                            mRaytracing = false;
    uint64_t                                        mBLAlignment = 0;
    std::vector<SPtr<RHI::AccelerationStructure>>   mBottomLevel;
    SPtr<RHI::Buffer>                               mBottomLevelData;

    [[nodiscard]] uint64_t alignBLAS(const uint64_t x) const noexcept
    {
        return (x + mBLAlignment - 1) & ~(mBLAlignment - 1);
    }

    // Updates
    // ========================
    std::vector<GeometryInfo>   mUploadQueue;

    // Resize buffers and upload queued geometry data
    void uploadQueuedData() noexcept;

    SPtr<RHI::VulkanRHI>        mRHI;
};
