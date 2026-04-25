#pragma once

#include "Geometry.hpp"
#include "GeometryInfo.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class GeometrySystem;

    /**
     * Handle to access geometry system buffers. Valid while the owning GeometrySystem is alive.
     */
    class GeometryBufferHandle
    {
    public:
        [[nodiscard]] const SPtr<RHI::Buffer>& getVertexBuffer() const noexcept;

        [[nodiscard]] const SPtr<RHI::Buffer>& getIndexBuffer() const noexcept;

        [[nodiscard]] const SPtr<RHI::Buffer>& getInfoBuffer() const noexcept;

    private:
        // Hide ctor, only GeometrySystem should be able to create a Handle.
        friend class GeometrySystem;

        explicit GeometryBufferHandle(GeometrySystem* pGeometrySystem)
        : mSystem(pGeometrySystem)
        {
            if (!mSystem)
            {
                exitWithError("GeometrySystem was null");
            }
        }

        GeometrySystem* mSystem = nullptr;
    };

    struct PendingRelease
    {
        SPtr<RHI::Buffer> buffer;
        uint64_t          frameToRelease;
    };

    class GeometrySystem
    {
    public:
        explicit GeometrySystem(const SPtr<RHI::VulkanRHI>& rhi);

        void onUpdate(const RHI::FrameData& frameData, const RHI::CommandList* pCommandList);

        [[nodiscard]] int32_t addGeometry(const SPtr<Geometry>& geometry)
        {
            mGeometries.push_back(geometry);
            const auto& g = mGeometries.back();

            const GeometryInfo info = {
                .geometryIndex = static_cast<int32_t>(mGeometries.size() - 1),
                .triangleCount = g->getTriangleCount(),
                .firstVertex   = mLastVertex,
                .vertexCount   = g->getVertexCount(),
                .firstIndex    = mLastIndex,
                .indexCount    = g->getIndexCount(),
            };
            mGeometryInfos.push_back(info);

            mLastVertex += info.vertexCount;
            mLastIndex  += info.indexCount;

            mUploadQueue.push_back(info.geometryIndex);

            return info.geometryIndex;
        }

        template <class T, class... Args>
        requires std::is_base_of_v<Geometry, T>
        [[nodiscard]] int32_t addGeometry(Args&&... args)
        {
            return addGeometry(makeShared<T>(std::forward<Args>(args)...));
        }

        [[nodiscard]] Geometry* getGeometry(int32_t index) const noexcept;

        [[nodiscard]] const GeometryInfo& getGeometryInfo(int32_t index) const noexcept;

        [[nodiscard]] GeometryBufferHandle getBuffers() noexcept;

        [[nodiscard]] uint32_t getGeometryCount() const noexcept;

        [[nodiscard]] const std::vector<GeometryInfo>& getGeometryInfos() const noexcept;

    private:
        // Let the Handle access buffers without getter methods.
        friend class GeometryBufferHandle;

        SPtr<RHI::VulkanRHI>        mRHI;

        // CPU-side Geometry Data and Trackers
        // ============================================================
        std::vector<SPtr<Geometry>> mGeometries     = {};
        std::vector<GeometryInfo>   mGeometryInfos  = {};

        // This value represents the last geometry that has been commited to the GPU-side buffers.
        uint32_t                    mCommitted      = 0;

        // GPU-side Geometry Data and Trackers
        // ============================================================
        uint32_t                    mLastVertex     = 0;
        SPtr<RHI::Buffer>           mVertexBuffer   = nullptr;
        uint32_t                    mLastIndex      = 0;
        SPtr<RHI::Buffer>           mIndexBuffer    = nullptr;
        SPtr<RHI::Buffer>           mInfoBuffer     = nullptr;

        // Upload Queue
        // ============================================================
        std::vector<int32_t>        mUploadQueue;
        SPtr<RHI::Buffer>           mStaging;
        std::vector<PendingRelease> mPendingReleases;
    };
}
