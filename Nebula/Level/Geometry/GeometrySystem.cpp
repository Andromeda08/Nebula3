#include "GeometrySystem.hpp"

namespace nbl
{
    const SPtr<RHI::Buffer>& GeometryBufferHandle::getVertexBuffer() const noexcept
    {
        return mSystem->mVertexBuffer;
    }

    const SPtr<RHI::Buffer>& GeometryBufferHandle::getIndexBuffer() const noexcept
    {
        return mSystem->mIndexBuffer;
    }

    const SPtr<RHI::Buffer>& GeometryBufferHandle::getInfoBuffer() const noexcept
    {
        return mSystem->mInfoBuffer;
    }

    GeometrySystem::GeometrySystem(const SPtr<RHI::VulkanRHI>& rhi)
    : mRHI(rhi)
    {
    }

    void GeometrySystem::onUpdate(const RHI::FrameData& frameData, const RHI::CommandList* pCommandList)
    {
        // Check buffers pending deletion
        for (auto it = mPendingReleases.begin(); it != mPendingReleases.end();)
        {
            if (mRHI->isFrameComplete(it->frameToRelease))
            {
                it = mPendingReleases.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (mUploadQueue.empty())
        {
            return;
        }

        // New data size
        uint64_t addVtxSize = 0;
        uint64_t addIdxSize = 0;
        for (const auto i : mUploadQueue)
        {
            addVtxSize += mGeometryInfos[i].vertexCount * sizeof(Vertex);
            addIdxSize += mGeometryInfos[i].indexCount  * sizeof(uint32_t);
        }

        // Create staging for incoming data
        mStaging = mRHI->createBuffer({
            .size  = addVtxSize + addIdxSize + (mGeometryInfos.size() * sizeof(GeometryInfo)),
            .type  = RHI::BufferType::Staging,
            .label = fmt::format("GeometrySystem_onUpdate_Staging_{}", frameData.currentFrame),
        });

        // Set staging data and prepare copies
        uint64_t vtxOffset = 0;
        uint64_t idxOffset = addVtxSize;
        for (const auto i : mUploadQueue)
        {
            const auto* g = mGeometries[i].get();

            mStaging->setData(g->getVertices().data(), mGeometryInfos[i].getVertexSize(), vtxOffset);
            mStaging->setData(g->getIndices().data(),  mGeometryInfos[i].getIndexSize(),  idxOffset);

            mStaging->setData(mGeometryInfos.data(), mGeometryInfos.size() * sizeof(GeometryInfo), addVtxSize + addIdxSize);

            vtxOffset += mGeometryInfos[i].getVertexSize();
            idxOffset += mGeometryInfos[i].getIndexSize();
        }

        const auto oldVtxSize = mVertexBuffer ? mVertexBuffer->getSize() : 0;
        const auto oldIdxSize = mIndexBuffer ? mIndexBuffer->getSize() : 0;

        const auto newVerticesCopy = vk::BufferCopy2()
            .setSrcOffset(0)
            .setDstOffset(oldVtxSize)
            .setSize(addVtxSize);

        const auto newIndicesCopy = vk::BufferCopy2()
            .setSrcOffset(addVtxSize)
            .setDstOffset(oldIdxSize)
            .setSize(addIdxSize);

        const auto newInfosCopy = vk::BufferCopy2()
            .setSrcOffset(addVtxSize + addIdxSize)
            .setDstOffset(0)
            .setSize(mGeometryInfos.size() * sizeof(GeometryInfo));

        RHI::Buffer* pOldVertexBuffer = nullptr;
        if (mVertexBuffer)
        {
            mPendingReleases.push_back({ mVertexBuffer, frameData.lifetimeFrameCounter + RHI::gFramesInFlight });
            pOldVertexBuffer = mPendingReleases.back().buffer.get();
        }

        RHI::Buffer* pOldIndexBuffer = nullptr;
        if (mIndexBuffer)
        {
            mPendingReleases.push_back({ mIndexBuffer, frameData.lifetimeFrameCounter + RHI::gFramesInFlight });
            pOldIndexBuffer = mPendingReleases.back().buffer.get();
        }

        if (mInfoBuffer)
        {
            mPendingReleases.push_back({ mInfoBuffer, frameData.lifetimeFrameCounter + RHI::gFramesInFlight });
        }

        mVertexBuffer = mRHI->createBuffer({
            .size  = oldVtxSize + addVtxSize,
            .type  = RHI::BufferType::Vertex,
            .label = "GeometrySystem_VertexBuffer",
        });
        mIndexBuffer = mRHI->createBuffer({
            .size  = oldIdxSize + addIdxSize,
            .type  = RHI::BufferType::Index,
            .label = "GeometrySystem_IndexBuffer",
        });
        mInfoBuffer = mRHI->createBuffer({
            .size  = mGeometryInfos.size() * sizeof(GeometryInfo),
            .type  = RHI::BufferType::Storage,
            .label = "GeometrySystem_InfoBuffer"
        });

        /* Record Commands */ {
            // Copy old vertex data to front of new buffer
            // ================================================
            if (pOldVertexBuffer)
            {
                #pragma region
                const auto oldVtxCopy     = vk::BufferCopy2 { 0, 0, oldVtxSize };
                const auto oldVtxCopyInfo = vk::CopyBufferInfo2()
                    .setSrcBuffer(pOldVertexBuffer->getHandle())
                    .setDstBuffer(mVertexBuffer->getHandle())
                    .setRegions(oldVtxCopy);
                #pragma endregion
                pCommandList->getHandle().copyBuffer2(oldVtxCopyInfo);
            }

            // Copy old index data to front of new buffer
            // ================================================
            if (pOldIndexBuffer)
            {
                #pragma region
                const auto oldIdxCopy     = vk::BufferCopy2 { 0, 0, oldIdxSize };
                const auto oldIdxCopyInfo = vk::CopyBufferInfo2()
                    .setSrcBuffer(pOldIndexBuffer->getHandle())
                    .setDstBuffer(mIndexBuffer->getHandle())
                    .setRegions(oldIdxCopy);
                #pragma endregion
                pCommandList->getHandle().copyBuffer2(oldIdxCopyInfo);
            }

            // Copy new data
            // ================================================
            const auto newVtxCopyInfo = vk::CopyBufferInfo2()
                .setSrcBuffer(mStaging->getHandle())
                .setDstBuffer(mVertexBuffer->getHandle())
                .setRegions(newVerticesCopy);
            pCommandList->getHandle().copyBuffer2(newVtxCopyInfo);

            const auto newIdxCopyInfo = vk::CopyBufferInfo2()
                .setSrcBuffer(mStaging->getHandle())
                .setDstBuffer(mIndexBuffer->getHandle())
                .setRegions(newIndicesCopy);
            pCommandList->getHandle().copyBuffer2(newIdxCopyInfo);

            const auto newInfoCopyInfo = vk::CopyBufferInfo2()
                .setSrcBuffer(mStaging->getHandle())
                .setDstBuffer(mInfoBuffer->getHandle())
                .setRegions(newInfosCopy);
            pCommandList->getHandle().copyBuffer2(newInfoCopyInfo);
        }

        mCommitted += mUploadQueue.size();
        mUploadQueue.clear();
    }

    Geometry* GeometrySystem::getGeometry(const int32_t index) const noexcept
    {
        return mGeometries[index].get();
    }

    const GeometryInfo& GeometrySystem::getGeometryInfo(const int32_t index) const noexcept
    {
        return mGeometryInfos[index];
    }

    GeometryBufferHandle GeometrySystem::getBuffers() noexcept
    {
        return GeometryBufferHandle(this);
    }

    uint32_t GeometrySystem::getGeometryCount() const noexcept
    {
        return static_cast<uint32_t>(mGeometries.size());
    }

    const std::vector<GeometryInfo>& GeometrySystem::getGeometryInfos() const noexcept
    {
        return mGeometryInfos;
    }
}
