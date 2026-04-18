#pragma once

#include <vector>

#include "Geometry.hpp"
#include "Core/Types.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    struct GeometrySystemConfig
    {
        bool generateMeshlets = true;
        bool createBLAS       = true;

        void checkSupport(const RHI::VulkanRHI* pRHI) noexcept;
    };

    /**
     * Geometry Indices are **stable** (since only additions are supported),
     * but only valid for rendering after they have been uploaded to the GPU via commit().
     */
    using GeometryIndex = int32_t;

    struct GeometryMetadata
    {
        // Vertex buffer region
        uint32_t        firstVertex;
        uint32_t        vertexCount;

        // Index buffer region
        uint32_t        firstIndex;
        uint32_t        indexCount;

        // Bottom-level AS Address
        uint64_t        blasAddress;

        // Geometry Meta
        GeometryIndex   geometryIndex;
        int32_t         _pad0;
    };

    class GeometrySystem
    {
    public:
        struct GeometryUploadTask
        {
            GeometryIndex index;
        };

        GeometrySystem(const GeometrySystemConfig& config, const SPtr<RHI::VulkanRHI>& rhi);

        template <class T, class... Args>
        requires std::is_base_of_v<Geometry, T>
        [[nodiscard]] GeometryIndex addGeometry(Args&&... args)
        {
            mGeometries.emplace_back(makeShared<T>(std::forward<Args>(args)...));
            const auto* pGeometry = mGeometries.back().get();
            const GeometryIndex index = static_cast<GeometryIndex>(mGeometries.size() - 1);

            const auto meta = GeometryMetadata {
                .firstVertex      = mLastVertex,
                .vertexCount      = pGeometry->getVertexCount(),
                .firstIndex       = mLastIndex,
                .indexCount       = pGeometry->getIndexCount(),
                .blasAddress      = 0,
                .geometryIndex    = index,
            };
            mMetadata.push_back(meta);

            mUploadTasks.push_back({
                .index = index,
            });

            mLastVertex  += pGeometry->getVertexCount();
            mLastIndex   += pGeometry->getIndexCount();

            return index;
        }

        [[nodiscard]] Geometry* getGeometry(GeometryIndex i) const noexcept;

        [[nodiscard]] const GeometryMetadata& getGeometryMeta(GeometryIndex i) const noexcept;

        // Execute the queued GPU upload tasks
        void commit();

    private:
        void uploadGeometries();

        friend class GeometrySystemDebugRenderPass;

        GeometrySystemConfig            mConfig;

        SPtr<RHI::VulkanRHI>            mRHI;

        std::vector<SPtr<Geometry>>     mGeometries;
        std::vector<GeometryMetadata>   mMetadata;

        std::vector<GeometryUploadTask> mUploadTasks;
        bool                            mFirstUpload = true;

        // GPU Buffers
        uint32_t                        mLastVertex  = 0;
        uint32_t                        mLastIndex   = 0;
        uint32_t                        mLastMeshlet = 0;

        SPtr<RHI::Buffer>               mVertexBuffer;
        SPtr<RHI::Buffer>               mAttributeBuffer;
        SPtr<RHI::Buffer>               mIndexBuffer;
        SPtr<RHI::Buffer>               mShadowIndexBuffer;

        SPtr<RHI::Buffer>               mMetaBuffer;
    };
}
