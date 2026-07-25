#include "BoundingBoxDebugPass.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "Templates.hpp"
#include "Level/Level.hpp"

// BoundingBoxDebugPass
// ======================================
namespace nbl
{
    BoundingBoxDebugPass::BoundingBoxDebugPass(const BoundingBoxDebugPass_Params& params)
    : mRHI(params.rhi)
    , mInput(params)
    {
        init();

        mInput.pLevel->mUserInterface->addComponent<BoundingBoxDebugPassUI>(this);
    }

    void BoundingBoxDebugPass::execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const SPtr<RHI::Image>& depthBuffer) const noexcept
    {
        if (!mVisualizeAABBs)
        {
            return;
        }

        RHI::Rendering()
            .setLabel("BoundingBoxVis_RenderPass")
            .setRenderArea(mInput.renderTargets[frameData.currentFrame]->getProperties().extent)
            .addAttachment(mInput.renderTargets[frameData.currentFrame], vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore)
            .addAttachment(depthBuffer, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eNone)
            .insertBarriers(pCommandList)
            .setViewportScissor(pCommandList)
            .execute(pCommandList, [&](RHI::CommandList* cmd) -> void
            {
                cmd->bindPipeline(mPipeline.get());

                auto pushConstant = PushConstants {
                    .boxColor       = mBoxColor,
                    .instanceBuffer = mInput.pLevel->mInstanceSystem->getBuffer()->getAddress(),
                    .cameraBuffer   = mInput.pLevel->mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
                    .instanceIndex  = 0 /* !! Set before draw calls !! */,
                };

                // Render for selected object
                const auto* selectedObject = mInput.pLevel->getSelectedObject();
                if (mFocusSelectedObject && selectedObject != nullptr)
                {
                    pushConstant.instanceIndex = mInput.pLevel->mInstanceSystem->getGpuIndex(selectedObject->hInstance);
                    cmd->pushConstants(&pushConstant);
                    cmd->getHandle().draw(24, 1, 0, 0);
                }
                // Render all bounding boxes
                else
                {
                    for (const auto& object : mInput.pLevel->getObjects())
                    {
                        pushConstant.instanceIndex = mInput.pLevel->mInstanceSystem->getGpuIndex(object->hInstance);
                        cmd->pushConstants(&pushConstant);
                        cmd->getHandle().draw(24, 1, 0, 0);
                    }
                }
            });
    }

    void BoundingBoxDebugPass::init() noexcept
    {
        const auto graphicsPS = RHI::GraphicsPS()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .setTopology(vk::PrimitiveTopology::eLineList)
            .addDefaultAttachmentState(1)
            .addAttachmentFormat(mInput.renderTargets[0]->getProperties().format)
            .addAttachmentFormat(vk::Format::eD32Sfloat);
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("BoundingBoxVis")
            .addShader("BoundingBoxDebug.vert.spv")
            .addShader("BoundingBoxDebug.frag.spv")
            .setPushConstant<PushConstants>(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

        mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }

    bool BoundingBoxDebugPass::hasSelectedObject() const
    {
        return mInput.pLevel->mSelectObjectFeature && *mInput.pLevel->mSelectObjectFeature->getSelectedObjectIdx() != -1;
    }
}

// BoundingBoxDebugPass UI Component
// ======================================
namespace nbl
{
    BoundingBoxDebugPassUI::BoundingBoxDebugPassUI(BoundingBoxDebugPass* pPass)
    : IComponent()
    , mPass(pPass)
    {
    }

    void BoundingBoxDebugPassUI::draw()
    {
        if (!mPass)
        {
            return;
        }

        ImGui::Begin("AABB Debug");

        ImGui::Checkbox("Render AABBs", &mPass->mVisualizeAABBs);
        ImGui::Checkbox("Selected Object", &mPass->mFocusSelectedObject);
        if (mPass->mFocusSelectedObject && !mPass->hasSelectedObject())
        {
            ImGui::TextColored(ImVec4(0.95f, 0.1f, 0.15f, 1.0f), "No object is selected.");
        }

        ImGui::ColorEdit4("Color", glm::value_ptr(mPass->mBoxColor));

        ImGui::End();
    }
}
