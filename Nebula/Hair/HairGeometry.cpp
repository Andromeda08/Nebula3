#include "HairGeometry.hpp"

#include "Utils.hpp"
#include "Core/Ranges.hpp"

namespace nbl
{
    HairModelSystem::HairModelSystem(const SPtr<RHI::VulkanRHI>& rhi)
    : mRHI(rhi)
    {
    }

    uint32_t HairModelSystem::addHairGeometry(const HairGeometry& hairGeometry)
    {
        mHairGeometries.push_back(hairGeometry);

        mHairInfos.push_back({
            .firstVertex    = mVertexCount,
            .vertexCount    = hairGeometry.getVertexCount(),
            .firstAttribute = mVertexCount,
            .attributeCount = hairGeometry.getVertexCount(),
            .firstStrand    = mStrandCount,
            .strandCount    = hairGeometry.getStrandCount(),
        });

        mVertexCount += hairGeometry.getVertexCount();
        mStrandCount += hairGeometry.getStrandCount();

        return static_cast<uint32_t>(mHairGeometries.size()) - 1;
    }

    const HairGeometry& HairModelSystem::getHairGeometry(const uint32_t i) const noexcept
    {
        return mHairGeometries[i];
    }

    void HairModelSystem::createBuffers()
    {
        // Allocate storage buffers
        // ================================================
        #pragma region

        mHairVertices = mRHI->createBuffer({
            .size  = mVertexCount * sizeof(HairVertex),
            .type  = RHI::BufferType::Storage,
            .label = "GlobalHairVertexBuffer",
        });
        mHairAttributes = mRHI->createBuffer({
            .size  = mVertexCount * sizeof(HairAttributes),
            .type  = RHI::BufferType::Storage,
            .label = "GlobalHairAttributeBuffer",
        });
        mStrandDescriptions = mRHI->createBuffer({
            .size  = mStrandCount * sizeof(HairStrandDesc),
            .type  = RHI::BufferType::Storage,
            .label = "GlobalHairStrandDescriptionBuffer",
        });
        mGlobalHairInfo = mRHI->createBuffer({
            .size  = mHairGeometries.size() * sizeof(GlobalHairInfo),
            .type  = RHI::BufferType::Storage,
            .label = "GlobalHairInfoBuffer",
        });

        #pragma endregion

        // Copy data to buffers
        // ================================================
        auto copyBatch = CopyBatchBuilder()
            .addCopy(mGlobalHairInfo.get(), mHairInfos.data(), mHairInfos.size() * sizeof(GlobalHairInfo));

        for (const auto& [index, hair] : enumerate(mHairGeometries))
        {
            const auto& info = mHairInfos[index];

            copyBatch.addCopy(
                mHairVertices.get(),
                hair.vertices.data(),
                hair.vertices.size() * sizeof(HairVertex),
                info.firstVertex * sizeof(HairVertex)
            )
           .addCopy(
                mHairAttributes.get(),
                hair.attributes.data(),
                hair.attributes.size() * sizeof(HairAttributes),
                info.firstAttribute * sizeof(HairAttributes)
           )
           .addCopy(
                mStrandDescriptions.get(),
                hair.strandDescs.data(),
                hair.strandDescs.size() * sizeof(HairStrandDesc),
                info.firstStrand * sizeof(HairStrandDesc)
            );
        }

        const auto stagingBuffer = mRHI->createBuffer({
            .size  = copyBatch.getStagingBufferSize(),
            .type  = RHI::BufferType::Staging,
            .label = "HairDataStaging",
        });

        copyBatch.setStagingData(stagingBuffer.get());

        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void
        {
            copyBatch.recordCopies(stagingBuffer.get(), pCommandList);
        });
    }
}
