#include "HairRenderer.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace nbl
{
    HairRenderer::HairRenderer(const SPtr<RHI::VulkanRHI>& rhi, HairModelSystem* pHairModelSystem)
    : mRHI(rhi)
    , mHairModelSystem(pHairModelSystem)
    {
        createStatsResources();

        mShared            = makeUnique<HairShared>(rhi.get(), mHairModelSystem);
        mMeshStage         = makeUnique<Hybrid_MeshStage>(mRHI, mShared.get());
        mSoftwareStage     = makeUnique<Hybrid_SoftwareStage>(mRHI, mShared.get());
    }

    HairRenderer::~HairRenderer()
    {
        for (size_t i = 0; i < RHI::gFramesInFlight; i++)
        {
            mRHI->getDevice()->getHandle().destroyQueryPool(mMeshPrimitivePool[i]);
        }
    }

    void HairRenderer::execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const HairRenderer_BDAs& buffers)
    {
        execute_getQueryPoolResults(frameData.currentFrame);

        pCommandList->beginLabel(mLabel);

        // Hardware Stage
        {
            execute_beginQuery(pCommandList, frameData.currentFrame);
            mMeshStage->execute(pCommandList, frameData, buffers);
            execute_endQuery(pCommandList, frameData.currentFrame);
        }

        // Software Stage
        if (mShared->config.isHybridMode)
        {
            mSoftwareStage->execute(pCommandList, frameData, buffers);
        }

        readbackSmallTriangleCount(pCommandList, frameData.currentFrame);

        pCommandList->endLabel();
    }

    void HairRenderer::drawUI()
    {
        constexpr uint32_t uint32min = 0;

        ImGui::SeparatorText("Config");

        const auto hairIndexMax = mHairModelSystem->getModelCount() - 1;
        ImGui::SliderScalar("Hair Model", ImGuiDataType_U32, &mShared->config.hairIndex, &uint32min, &hairIndexMax);

        ImGui::Checkbox("Render Head", &mShared->config.renderHead);
        ImGui::ColorEdit3("Head Color", glm::value_ptr(mShared->config.headColor));

        ImGui::Checkbox("Enable Hybrid Mode", &mShared->config.isHybridMode);

        ImGui::SeparatorText("Statistics");
        {
            const auto percent = (static_cast<float>(mSmallTriangles) / static_cast<float>(mMeshTriangles)) * 100.0f;

            ImGui::Text("Mesh Triangles: %llu", mMeshTriangles);
            ImGui::Text("Small Triangles: %llu (%.2f%%)", mSmallTriangles, percent);
            ImGui::DragFloat("Small Triangle Threshold", &mShared->config.smallTriangleThreshold, 0.1f, 0.0f, 8.0f);

        }

        ImGui::SeparatorText("Task Workgroup Size");
        {
            const auto defaultGroupSize = mHairModelSystem->getHairGeometry(mShared->config.hairIndex).taskGroupSizeX;

            ImGui::Text("Default size: %u", defaultGroupSize);
            ImGui::Checkbox("Override", &mShared->config.useCustomWgSize);
            ImGui::SliderScalar("X", ImGuiDataType_U32, &mShared->config.customTaskWgSize, &uint32min, &defaultGroupSize);
        }

        ImGui::SeparatorText("Marschner BSDF");
        {
            ImGui::ColorEdit3("Diffuse Tint", glm::value_ptr(mShared->config.bsdfParams.diffuseTint));
            ImGui::ColorEdit3("Specular Tint", glm::value_ptr(mShared->config.bsdfParams.specularTint));

            ImGui::DragFloat("Scale R",   &mShared->config.bsdfParams.scaleR,   0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Scale TT",  &mShared->config.bsdfParams.scaleTT,  0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Scale TRT", &mShared->config.bsdfParams.scaleTRT, 0.01f, 0.0f, 1.0f);

            ImGui::SliderAngle("Shift R",   &mShared->config.bsdfParams.shiftR);
            ImGui::SliderAngle("Shift TT",  &mShared->config.bsdfParams.shiftTT);
            ImGui::SliderAngle("Shift TRT", &mShared->config.bsdfParams.shiftTRT);
        }
    }

    const SPtr<RHI::Image>& HairRenderer::getResult(const uint32_t currentFrame)
    {
        return mShared->colorTarget[currentFrame];
    }

    const SPtr<RHI::Image>& HairRenderer::getDepth(const uint32_t currentFrame)
    {
        return mShared->depthBuffer[currentFrame];
    }

    void HairRenderer::execute_getQueryPoolResults(const uint32_t currentFrame)
    {
        if (mMeshPrimitiveQueryValid[currentFrame])
        {
            uint64_t result = 0;
            const auto res = mRHI->getDevice()->getHandle().getQueryPoolResults(
                mMeshPrimitivePool[currentFrame],
                0, 1,
                sizeof(uint64_t), &result, sizeof(uint64_t),
                vk::QueryResultFlagBits::e64
            );

            if (res == vk::Result::eSuccess)
            {
                mMeshTriangles = result;
            }
        }
    }

    void HairRenderer::execute_beginQuery(const RHI::CommandList* pCommandList, const uint32_t currentFrame) const
    {
        const auto& pool = mMeshPrimitivePool[currentFrame];
        pCommandList->getHandle().resetQueryPool(pool, 0, 1);
        pCommandList->getHandle().beginQuery(pool, 0, {});
    }

    void HairRenderer::execute_endQuery(const RHI::CommandList* pCommandList, const uint32_t currentFrame)
    {
        const auto& pool = mMeshPrimitivePool[currentFrame];
        pCommandList->getHandle().endQuery(pool, 0);
        mMeshPrimitiveQueryValid[currentFrame] = true;
    }

    void HairRenderer::readbackSmallTriangleCount(const RHI::CommandList* pCommandList, const uint32_t currentFrame)
    {
        RHI::Barrier()
            .addBarrier(mShared->smallTriangleCounterBuffer[currentFrame]->getBarrier(RHI::BufferUsage::Compute_Read, RHI::BufferUsage::TransferSrc))
            .addBarrier(mReadBackTriCount[currentFrame]->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::TransferDst))
            .insert(pCommandList);

        constexpr auto copyTriCount = vk::BufferCopy2 { 0, 0, sizeof(uint32_t) };
        const auto copyInfo = vk::CopyBufferInfo2()
            .setSrcBuffer(mShared->smallTriangleCounterBuffer[currentFrame]->getHandle())
            .setDstBuffer(mReadBackTriCount[currentFrame]->getHandle())
            .setRegions(copyTriCount);
        pCommandList->getHandle().copyBuffer2(copyInfo);

        RHI::Barrier()
            .addBarrier(mReadBackTriCount[currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::Host_Read))
            .insert(pCommandList);

        uint32_t data = 0;
        mReadBackTriCount[currentFrame]->readBack(&data, sizeof(uint32_t), 0);

        mSmallTriangles = static_cast<uint64_t>(data);
    }

    void HairRenderer::createStatsResources()
    {
        constexpr auto queryInfo = vk::QueryPoolCreateInfo()
            .setQueryType(vk::QueryType::eMeshPrimitivesGeneratedEXT)
            .setQueryCount(1);

        for (size_t i = 0; i < RHI::gFramesInFlight; i++)
        {
            mMeshPrimitivePool[i] = mRHI->getDevice()->getHandle().createQueryPool(queryInfo);
            mMeshPrimitiveQueryValid[i] = false;

            mReadBackTriCount[i] = mRHI->createBuffer({
                .size  = sizeof(uint64_t),
                .type  = RHI::BufferType::Readback,
                .label = fmt::format("Hair_TriangleCount_Readback_{}", i),
            });
        }
    }
}
