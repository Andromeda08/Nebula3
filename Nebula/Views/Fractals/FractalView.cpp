#include "FractalView.hpp"

#include "Level/Render/Templates.hpp"

namespace nbl
{
    FractalView::FractalView(GamepadManager* pGamepad, const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager, UserInterface* pUserInterface)
    : nbl_ViewBaseCtor
    {
        mName = "Fractals";

        /* Create Render Targets and Pipeline */
        {
            for (size_t i = 0; i < mTargets.size(); i++)
            {
                mTargets[i] = makeRenderTarget(mRHI.get(), fmt::format("FractalTarget_{}", i));
            }

            const auto graphicsPS = RHI::GraphicsPS()
                .setCullMode(vk::CullModeFlagBits::eNone)
                .addDefaultAttachmentState(1)
                .addAttachmentFormat(mTargets[0]->getProperties().format);
            const auto pipelineInfo = RHI::PipelineCommon()
                .setLabel("Fractals")
                .addShader("FSQuad.vert.spv")
                .addShader("Fractals.frag.spv");

            mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
        }
    }

}
