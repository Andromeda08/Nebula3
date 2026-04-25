#include "BLASSystem.hpp"

#include "Core/Ranges.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace nbl
{
    BLASSystem::BLASSystem(const SPtr<RHI::VulkanRHI>& rhi, GeometrySystem* pGeometrySystem)
    : mRHI(rhi)
    , mGeometrySystem(pGeometrySystem)
    {
    }

    void BLASSystem::onUpdate(const RHI::FrameData& frameData, const RHI::CommandList* pCommandList)
    {
        // Buffers
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

        // AS
        for (auto it = mPendingASRelease.begin(); it != mPendingASRelease.end();)
        {
            if (mRHI->isFrameComplete(it->frameToRelease))
            {
                it = mPendingASRelease.erase(it);
            }
            else
            {
                ++it;
            }
        }

        const auto device = mRHI->getDevice()->getHandle();
        const auto geomBuffers = mGeometrySystem->getBuffers();
        const auto buildIndices = collectBuildGeometryIndices();
        if (buildIndices.empty())
        {
            return;
        }

        pCommandList->beginLabel("BLAS Build");

        const auto oldBlasCount = mBottomLevel.size();
        const auto addBlasCount = buildIndices.size();
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

        for (auto&& [queueIdx, geometryIndex] : enumerate(buildIndices))
        {
            const auto& geometryInfo = mGeometrySystem->getGeometryInfo(geometryIndex);

            const auto buildRangeInfo = vk::AccelerationStructureBuildRangeInfoKHR()
                .setFirstVertex(0)
                .setPrimitiveCount(geometryInfo.triangleCount)
                .setPrimitiveOffset(geometryInfo.firstIndex * sizeof(uint32_t))
                .setTransformOffset(0);
            buildRangeInfos.push_back(buildRangeInfo);

            const auto triangleData = vk::AccelerationStructureGeometryTrianglesDataKHR()
                .setIndexData(geomBuffers.getIndexBuffer()->getAddress())
                .setIndexType(vk::IndexType::eUint32)
                .setVertexData(geomBuffers.getVertexBuffer()->getAddress() + (geometryInfo.firstVertex * sizeof(Vertex)))
                .setVertexFormat(vk::Format::eR32G32B32Sfloat)
                .setVertexStride(sizeof(Vertex))
                .setMaxVertex(geometryInfo.vertexCount)
                .setPNext(nullptr);
            triangleDatas.push_back(triangleData);

            const auto geometryData = vk::AccelerationStructureGeometryDataKHR().setTriangles(triangleDatas.back());

            const auto geometry = vk::AccelerationStructureGeometryKHR()
                .setGeometryType(vk::GeometryTypeKHR::eTriangles)
                .setGeometry(geometryData);
            geometries.push_back(geometry);

            const auto buildGeometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
                .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowDataAccess)
                .setGeometryCount(1)
                .setGeometries(geometries.back())
                .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
                .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
            buildGeometryInfos.push_back(buildGeometryInfo);
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
            [](uint64_t acc, const auto& bsi){ return acc + alignBLAS(bsi.accelerationStructureSize); });

        auto newBottomLevelData = mRHI->createBuffer({
            .size  = oldBufferSize + addBufferSize,
            .type  = RHI::BufferType::AccelerationStructure,
            .label = "BottomLevelAS_Data",
        });
        #pragma endregion

        mStaging = mRHI->createBuffer({
            .size  = addBufferSize,
            .type  = RHI::BufferType::Storage,
            .label = "BottomLevelAS_Scratch",
        });

        // Prepare new bottom level build
        // ================================================
        #pragma region
        std::vector<SPtr<RHI::AccelerationStructure>> newBottomLevel(newBlasCount);

        // Copy old BLAS
        if (oldBlasCount > 0)
        {
            RHI::Barrier()
                .addBarrier(mBottomLevelData->getBarrier(RHI::BufferUsage::AS_BuildUpdate, RHI::BufferUsage::AS_BuildUpdate))
                .insert(pCommandList);

            for (auto i = 0; i < oldBlasCount; i++)
            {
                newBottomLevel[i] = RHI::AccelerationStructure::create({
                    .backingBuffer = newBottomLevelData,
                    .offset = mBottomLevel[i]->getOffset(),
                    .size = mBottomLevel[i]->getSize(),
                    .type = RHI::AccelerationStructureType::BottomLevel,
                }, mRHI->getDevice());
            }

            for (auto&& [i, blas] : nbl::enumerate(mBottomLevel))
            {
                const auto copyInfo = vk::CopyAccelerationStructureInfoKHR()
                    .setSrc(blas->getHandle())
                    .setDst(newBottomLevel[i]->getHandle())
                    .setMode(vk::CopyAccelerationStructureModeKHR::eClone);
                pCommandList->getHandle().copyAccelerationStructureKHR(copyInfo);
            }
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
                .setScratchData(mStaging->getAddress() + stagingOffsets[i]);
        }

        RHI::Barrier()
            .addBarrier(newBottomLevelData->getBarrier(RHI::BufferUsage::AS_BuildUpdate, RHI::BufferUsage::AS_BuildUpdate))
            .addBarrier(geomBuffers.getVertexBuffer()->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::AS_BuildInput))
            .addBarrier(geomBuffers.getIndexBuffer()->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::AS_BuildInput))
            .insert(pCommandList);

        pCommandList->getHandle().buildAccelerationStructuresKHR(pBuildRangeInfos.size(), buildGeometryInfos.data(), pBuildRangeInfos.data());

        mPendingReleases.push_back({ mBottomLevelData, frameData.lifetimeFrameCounter + RHI::gFramesInFlight });
        for (const auto& as : mBottomLevel)
        {
            mPendingASRelease.push_back({ as, frameData.lifetimeFrameCounter + RHI::gFramesInFlight });
        }

        mBottomLevel     = std::move(newBottomLevel);
        mBottomLevelData = std::move(newBottomLevelData);

        for (auto idx : buildIndices)
        {
            mHasBlas.insert(idx);
        }

        spdlog::debug("BLAS count: {} -> {}", oldBlasCount, newBlasCount);

        RHI::Barrier()
            .addBarrier(mBottomLevelData->getBarrier(RHI::BufferUsage::AS_BuildUpdate, RHI::BufferUsage::AS_Traverse))
            .insert(pCommandList);

        pCommandList->endLabel();
    }

    uint64_t BLASSystem::getGeometryBlasAddress(const int32_t index) const noexcept
    {
        return mHasBlas.contains(index) ? mBottomLevel[index]->getAddress() : 0;
    }

    std::vector<int32_t> BLASSystem::collectBuildGeometryIndices() const
    {
        std::vector<int32_t> result;
        for (const auto& info : mGeometrySystem->getGeometryInfos())
        {
            if (!mHasBlas.contains(info.geometryIndex))
            {
                result.push_back(info.geometryIndex);
            }
        }
        return result;
    }
}
