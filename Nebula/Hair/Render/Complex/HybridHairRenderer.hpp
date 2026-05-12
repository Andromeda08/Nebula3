#pragma once

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "HybridHair_DebugCompose.hpp"
#include "HybridHair_MeshStage.hpp"
#include "HybridHair_SoftwareStage.hpp"
#include "Shared.hpp"
#include "Hair/HairGeometry.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class HybridHairRenderer
    {
    public:
        HybridHairRenderer(const SPtr<RHI::VulkanRHI>& rhi, HairModelSystem* pHairModelSystem)
        : mRHI(rhi)
        , mHairModelSystem(pHairModelSystem)
        {
            createStatsResources();

            mShared            = makeUnique<HairShared>(rhi.get(), mHairModelSystem);
            mMeshStage         = makeUnique<HybridHair_MeshStage>(mRHI, mShared.get());
            mSoftwareStage     = makeUnique<HybridHair_SoftwareStage>(mRHI, mShared.get());
            mDebugComposeStage = makeUnique<HybridHair_DebugCompose>(mRHI, mShared.get());
        }

        void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const uint64_t cameraBufferAddress)
        {
            if (mMeshPrimitiveQueryValid[frameData.currentFrame])
            {
                uint64_t result = 0;
                const auto res = mRHI->getDevice()->getHandle().getQueryPoolResults(
                    mMeshPrimitivePool[frameData.currentFrame],
                    0, 1,
                    sizeof(uint64_t), &result, sizeof(uint64_t),
                    vk::QueryResultFlagBits::e64
                );

                if (res == vk::Result::eSuccess)
                {
                    mMeshTriangles = result;
                }
            }

            pCommandList->beginLabel("HybridHairRenderer");

            const auto& pool = mMeshPrimitivePool[frameData.currentFrame];
            pCommandList->getHandle().resetQueryPool(pool, 0, 1);
            pCommandList->getHandle().beginQuery(pool, 0, {});

            // ============================================
            mMeshStage->execute(pCommandList, frameData, cameraBufferAddress);
            // ============================================

            pCommandList->getHandle().endQuery(pool, 0);
            mMeshPrimitiveQueryValid[frameData.currentFrame] = true;

            // ============================================
            mSoftwareStage->execute(pCommandList, frameData);
            // ============================================

            readbackSmallTriangleCount(pCommandList, frameData.currentFrame);

            // ============================================
            mDebugComposeStage->execute(pCommandList, frameData, mMeshStage->getResult(frameData.currentFrame), mSoftwareStage->getResult(frameData.currentFrame));
            // ============================================

            pCommandList->endLabel();
        }

        [[nodiscard]] const SPtr<RHI::Image>& getResult(const uint32_t currentFrame) const noexcept
        {
            return mDebugComposeStage->getResult(currentFrame);
        }

    private:
        void readbackSmallTriangleCount(const RHI::CommandList* pCommandList, const uint32_t currentFrame)
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

        void createStatsResources()
        {
            const auto queryInfo = vk::QueryPoolCreateInfo()
                .setQueryType(vk::QueryType::eMeshPrimitivesGeneratedEXT)
                .setQueryCount(1);

            for (size_t i = 0; i < RHI::gFramesInFlight; i++)
            {
                mMeshPrimitivePool[i] = mRHI->getDevice()->getHandle().createQueryPool(queryInfo);
                mMeshPrimitiveQueryValid[i] = false;

                mReadBackTriCount[i] = mRHI->createBuffer({
                    .size  = sizeof(uint64_t),
                    .type  = RHI::BufferType::Readback,
                    .label = fmt::format("HybridHair_TriangleCount_Readback_{}", i),
                });
            }
        }

        friend class HybridHairRendererUI;

        SPtr<RHI::VulkanRHI>             mRHI;
        HairModelSystem*                 mHairModelSystem;

        UPtr<HairShared>                 mShared;

        UPtr<HybridHair_MeshStage>       mMeshStage;
        UPtr<HybridHair_SoftwareStage>   mSoftwareStage;
        UPtr<HybridHair_DebugCompose>    mDebugComposeStage;

        PerFrameArray<vk::QueryPool>     mMeshPrimitivePool;
        PerFrameArray<bool>              mMeshPrimitiveQueryValid {};
        PerFrameArray<SPtr<RHI::Buffer>> mReadBackTriCount;

        uint64_t                         mMeshTriangles  = 0;
        uint64_t                         mSmallTriangles = 0;
    };

    class HybridHairRendererUI : public IComponent
    {
    public:
        explicit HybridHairRendererUI(HybridHairRenderer* pHairRenderer)
        : IComponent()
        , mHairRenderer(pHairRenderer)
        , mShared(pHairRenderer->mShared.get())
        {
        }

        void draw() override
        {
            constexpr uint32_t uint32min = 0;

            ImGui::Begin("HairRenderer");

            const auto hairIndexMax = mHairRenderer->mHairModelSystem->getModelCount() - 1;
            ImGui::SliderScalar("Hair Model", ImGuiDataType_U32, &mShared->config.hairIndex, &uint32min, &hairIndexMax);

            ImGui::SeparatorText("Hybrid Config & Stats");

            const auto percent = (static_cast<float>(mHairRenderer->mSmallTriangles) / static_cast<float>(mHairRenderer->mMeshTriangles)) * 100.0f;

            ImGui::Text("Mesh Triangles: %llu", mHairRenderer->mMeshTriangles);
            ImGui::Text("Small Triangles: %llu (%.2f%%)", mHairRenderer->mSmallTriangles, percent);
            ImGui::DragFloat("Alpha", &mShared->config.debugAlphaBlend, 0.05f, 0.0f, 1.0f);
            ImGui::DragFloat("Small Triangle Threshold", &mShared->config.smallTriangleThreshold, 0.1f, 0.0f, 8.0f);

            ImGui::SeparatorText("Task Workgroup Size");

            const auto defaultGroupSize = mHairRenderer->mHairModelSystem->getHairGeometry(mShared->config.hairIndex).taskGroupSizeX;

            ImGui::Text("Default size: %u", defaultGroupSize);
            ImGui::Checkbox("Override", &mShared->config.useCustomWgSize);
            ImGui::SliderScalar("X", ImGuiDataType_U32, &mShared->config.customTaskWgSize, &uint32min, &defaultGroupSize);

            ImGui::SeparatorText("Custom Color");
            ImGui::Checkbox("Enable", &mShared->config.overrideColors);
            ImGui::ColorEdit3("Diffuse", glm::value_ptr(mShared->config.diffuse));
            ImGui::ColorEdit3("Specular", glm::value_ptr(mShared->config.specular));
            ImGui::DragFloat("Specular", &mShared->config.specularFactor, 0.05f, 0.0f, 32.0f);

            ImGui::End();
        }

    private:
        HybridHairRenderer* mHairRenderer;
        HairShared*         mShared;
    };
}
