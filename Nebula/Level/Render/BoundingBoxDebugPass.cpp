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

    void BoundingBoxDebugPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        if (!mVisualizeAABBs)
        {
            return;
        }

        pCommandList->beginLabel("BoundingBoxDebugPass");

        pCommandList->setViewportScissor(mViewport, mScissor);

        RHI::Barrier()
            .addBarrier(mInput.renderTarget->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mInput.gBufferDepthBuffer->getBarrier(RHI::ImageUsage::DepthAttachment))
            .insert(pCommandList);

        mRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void
        {
            mPipeline->bind(cmd);

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
                mPipeline->pushConstants(cmd, &pushConstant);
                cmd->getHandle().draw(24, 1, 0, 0);
            }
            // Render all bounding boxes
            else
            {
                for (const auto& object : mInput.pLevel->getObjects())
                {
                    pushConstant.instanceIndex = mInput.pLevel->mInstanceSystem->getGpuIndex(object->hInstance);
                    mPipeline->pushConstants(cmd, &pushConstant);
                    cmd->getHandle().draw(24, 1, 0, 0);
                }
            }
        });

        pCommandList->endLabel();
    }

    void BoundingBoxDebugPass::init() noexcept
    {
        mScissor = getRenderAreaForAttachment(mInput.renderTarget.get());
        mViewport = vk::Viewport {
            0.0f, 0.0f,
            static_cast<float>(mScissor.extent.width), static_cast<float>(mScissor.extent.height),
            0.0f, 1.0f
        };

        mRenderPass = mRHI->createRenderPass({
            .renderArea       = mScissor,
            .colorAttachments = { makeAttachment(mInput.renderTarget, vk::AttachmentLoadOp::eLoad) },
            .depthAttachment  = makeAttachment(mInput.gBufferDepthBuffer, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eNone),
            .label            = "BoundingBoxDebug_RenderPass",
        });

        const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .setPushConstantRange<PushConstants>(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .setStateInfo(RHI::makeGraphicsStateInfo([&](RHI::GraphicsPipelineStateInfo& stateInfo)
            {
                stateInfo
                    .addDefaultAttachmentStates(1)
                    .setCullMode(vk::CullModeFlagBits::eNone)
                    .setTopology(vk::PrimitiveTopology::eLineList);
            }))
            .addShader({ Configuration::getShaderFilePath("BoundingBoxDebug.vert.spv"), vk::ShaderStageFlagBits::eVertex })
            .addShader({ Configuration::getShaderFilePath("BoundingBoxDebug.frag.spv"), vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mInput.renderTarget->getProperties().format)
            .setDepthAttachmentFormat(mInput.gBufferDepthBuffer->getProperties().format)
            .setDebugName("BoundingBoxDebug_Pipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }

    bool BoundingBoxDebugPass::hasSelectedObject() const
    {
        return *mInput.pLevel->mSelectObjectFeature->getSelectedObjectIdx() != -1;
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
