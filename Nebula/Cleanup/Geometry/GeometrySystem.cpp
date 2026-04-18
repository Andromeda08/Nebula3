#include "GeometrySystem.hpp"

#include <spdlog/fmt/bundled/color.h>
#include "Core/Ranges.hpp"

namespace nbl
{
    void GeometrySystemConfig::checkSupport(const RHI::VulkanRHI* pRHI) noexcept
    {
        if (!pRHI->getMeshShaderSupport())
        {
            spdlog::warn("GeometrySystem: Meshlet generation was requested, but the current RHI doesn't Mesh Shaders.");
        }
        if (!pRHI->getRaytracingSupport())
        {
            createBLAS = false;
            spdlog::warn("GeometrySystem: Bottom-level AS were requested, but the current RHI doesn't support it, {}.",
                         styled("the feature will be disabled", fmt::emphasis::bold));
        }
    }

    GeometrySystem::GeometrySystem(const GeometrySystemConfig& config, const SPtr<RHI::VulkanRHI>& rhi)
    : mConfig(config)
    , mRHI(rhi)
    {
    }

    Geometry* GeometrySystem::getGeometry(const GeometryIndex i) const noexcept
    {
        if (i >= mGeometries.size())
        {
            return nullptr;
        }
        return mGeometries[i].get();
    }

    const GeometryMetadata& GeometrySystem::getGeometryMeta(GeometryIndex i) const noexcept
    {
        return mMetadata[i];
    }

    void GeometrySystem::commit()
    {
        if (!mUploadTasks.empty())
        {
            uploadGeometries();
        }
    }

    void GeometrySystem::uploadGeometries()
    {
        uint64_t uploadVertexSize     = 0;
        uint64_t uploadAttribSize     = 0;
        uint64_t uploadIndexSize      = 0;
        for (const auto& [i, task] : nbl::enumerate(mUploadTasks))
        {
            uploadVertexSize += mMetadata[task.index].vertexCount * sizeof(Vertex);
            uploadAttribSize += mMetadata[task.index].vertexCount * sizeof(VertexAttributes);
            uploadIndexSize  += mMetadata[task.index].indexCount  * sizeof(IndexType);
        }

        const auto stagingBuffer = mRHI->createBuffer({
            uploadVertexSize + uploadAttribSize + uploadIndexSize + uploadIndexSize,
            RHI::BufferType::Staging, "GeometrySystem::uploadGeometries()::stagingBuffer"
        });

        // Record copies and upload to staging
        // ============================================
        #pragma region

        const uint64_t oldVertexSize = mVertexBuffer ? mVertexBuffer->getSize() : 0;
        std::vector<vk::BufferCopy2> vertexCopies;

        const uint64_t oldAttribSize = mAttributeBuffer ? mAttributeBuffer->getSize() : 0;
        std::vector<vk::BufferCopy2> attribCopies;

        const uint64_t oldIndexSize = mIndexBuffer ? mIndexBuffer->getSize() : 0;
        std::vector<vk::BufferCopy2> indexCopies;

        const uint64_t oldShadowIndexSize = mShadowIndexBuffer ? mShadowIndexBuffer->getSize() : 0;
        std::vector<vk::BufferCopy2> shadowIndexCopies;

        uint64_t stagingVertexOffset = 0;
        uint64_t stagingAttribOffset = uploadVertexSize;
        uint64_t stagingIndexOffset  = stagingAttribOffset + uploadAttribSize;
        uint64_t stagingShadowOffset = stagingIndexOffset + uploadIndexSize;

        for (const auto& task : mUploadTasks)
        {
            const auto* pGeometry = mGeometries[task.index].get();
            const auto& meta      = mMetadata[task.index];

            // Vertex Buffer
            const auto vertexSize = meta.vertexCount * sizeof(Vertex);
            vertexCopies.emplace_back(stagingVertexOffset, oldVertexSize + stagingVertexOffset, vertexSize);
            stagingBuffer->setData(pGeometry->getVertices().data(), vertexSize, stagingVertexOffset);

            stagingVertexOffset += vertexSize;

            // Attributes Buffer
            const auto attribSize = meta.vertexCount * sizeof(VertexAttributes);
            attribCopies.emplace_back(stagingAttribOffset, oldAttribSize + stagingAttribOffset - uploadVertexSize, attribSize);
            stagingBuffer->setData(pGeometry->getVertexAttributes().data(), attribSize, stagingAttribOffset);

            stagingAttribOffset += attribSize;

            // Index Buffer
            const auto indexSize = meta.indexCount * sizeof(IndexType);
            indexCopies.emplace_back(stagingIndexOffset, oldIndexSize + stagingIndexOffset - uploadVertexSize - uploadAttribSize, indexSize);
            stagingBuffer->setData(pGeometry->getIndices().data(), indexSize, stagingIndexOffset);

            stagingIndexOffset += indexSize;

            // Shadow Index Buffer
            const auto shadowSize = meta.indexCount * sizeof(IndexType);
            shadowIndexCopies.emplace_back(stagingShadowOffset, oldShadowIndexSize + stagingShadowOffset - uploadVertexSize - uploadAttribSize - uploadIndexSize, shadowSize);
            stagingBuffer->setData(pGeometry->getShadowIndices().data(), shadowSize, stagingShadowOffset);

            stagingShadowOffset += shadowSize;
        }

        #pragma endregion

        // Alloc new buffers
        // ============================================
        #pragma region

        auto newVertexBuffer = mRHI->createBuffer({
            .size  = oldVertexSize + uploadVertexSize,
            .type  = RHI::BufferType::Vertex,
            .label = "GeometrySystem_VertexBuffer",
        });
        auto newAttribBuffer = mRHI->createBuffer({
            .size  = oldAttribSize + uploadAttribSize,
            .type  = RHI::BufferType::Vertex,
            .label = "GeometrySystem_AttributeBuffer",
        });
        auto newIndexBuffer = mRHI->createBuffer({
            .size  = oldIndexSize + uploadIndexSize,
            .type  = RHI::BufferType::Index,
            .label = "GeometrySystem_IndexBuffer",
        });
        auto newShadowIndexBuffer = mRHI->createBuffer({
            .size  = oldIndexSize + uploadIndexSize,
            .type  = RHI::BufferType::Index,
            .label = "GeometrySystem_ShadowIndexBuffer",
        });

        #pragma endregion

        // Execute copies
        // ============================================
        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void
        {
            // Copy old buffers
            // ================================================
            if (mVertexBuffer)
            {
                #pragma region
                const auto oldVtxCopy = vk::BufferCopy2()
                    .setSrcOffset(0)
                    .setDstOffset(0)
                    .setSize(oldVertexSize);
                const auto oldVtxCopyInfo = vk::CopyBufferInfo2()
                    .setSrcBuffer(mVertexBuffer->getHandle())
                    .setDstBuffer(newVertexBuffer->getHandle())
                    .setRegions(oldVtxCopy);
                #pragma endregion
                pCommandList->getHandle().copyBuffer2(oldVtxCopyInfo);
            }
            if (mAttributeBuffer)
            {
                #pragma region
                const auto oldAttribCopy = vk::BufferCopy2()
                    .setSrcOffset(0)
                    .setDstOffset(0)
                    .setSize(oldAttribSize);
                const auto oldVtxCopyInfo = vk::CopyBufferInfo2()
                    .setSrcBuffer(mAttributeBuffer->getHandle())
                    .setDstBuffer(newAttribBuffer->getHandle())
                    .setRegions(oldAttribCopy);
                #pragma endregion
                pCommandList->getHandle().copyBuffer2(oldVtxCopyInfo);
            }
            if (mIndexBuffer)
            {
                #pragma region
                const auto oldIdxCopy = vk::BufferCopy2()
                    .setSrcOffset(0)
                    .setDstOffset(0)
                    .setSize(oldIndexSize);
                const auto oldVtxCopyInfo = vk::CopyBufferInfo2()
                    .setSrcBuffer(mIndexBuffer->getHandle())
                    .setDstBuffer(newIndexBuffer->getHandle())
                    .setRegions(oldIdxCopy);
                #pragma endregion
                pCommandList->getHandle().copyBuffer2(oldVtxCopyInfo);
            }
            if (mShadowIndexBuffer)
            {
                #pragma region
                const auto oldShIdxCopy = vk::BufferCopy2()
                    .setSrcOffset(0)
                    .setDstOffset(0)
                    .setSize(oldShadowIndexSize);
                const auto oldVtxCopyInfo = vk::CopyBufferInfo2()
                    .setSrcBuffer(mShadowIndexBuffer->getHandle())
                    .setDstBuffer(newShadowIndexBuffer->getHandle())
                    .setRegions(oldShIdxCopy);
                #pragma endregion
                pCommandList->getHandle().copyBuffer2(oldVtxCopyInfo);
            }

            // Copy new data
            // ================================================
            const auto newVtxCopyInfo = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(newVertexBuffer->getHandle())
                .setRegions(vertexCopies);
            pCommandList->getHandle().copyBuffer2(newVtxCopyInfo);

            const auto newAttrCopyInfo = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(newAttribBuffer->getHandle())
                .setRegions(attribCopies);
            pCommandList->getHandle().copyBuffer2(newAttrCopyInfo);

            const auto newIdxCopyInfo = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(newIndexBuffer->getHandle())
                .setRegions(indexCopies);
            pCommandList->getHandle().copyBuffer2(newIdxCopyInfo);

            const auto newShIdxCopyInfo = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(newShadowIndexBuffer->getHandle())
                .setRegions(shadowIndexCopies);
            pCommandList->getHandle().copyBuffer2(newShIdxCopyInfo);
        });

        mVertexBuffer      = std::move(newVertexBuffer);
        mAttributeBuffer   = std::move(newAttribBuffer);
        mIndexBuffer       = std::move(newIndexBuffer);
        mShadowIndexBuffer = std::move(newShadowIndexBuffer);

        mUploadTasks.clear();
    }
}
