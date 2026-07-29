#include "HairView.hpp"

#include "Hair/CyLoader.hpp"
#include "Level/Camera/OrbitCamera.hpp"

namespace nbl
{
    HairView::HairView(nbl_ViewCtorParams)
    : nbl_ViewBaseCtor
    {
        mName = "HairView";

        mCameraSystem = makeUnique<CameraSystem>(mRHI);
        mCameraSystem->addCamera<OrbitCamera>(true);

        /* Load Hair Models */ {
            mHairModelSystem = makeUnique<HairModelSystem>(mRHI);
            for (const auto& file : std::filesystem::directory_iterator(Configuration::getHairDir()))
            {
                if (file.path().extension() != ".hair")
                {
                    continue;
                }

                try
                {
                    const uint32_t hairIndex = mHairModelSystem->addHairGeometry(nbl::CyLoader(file).load());
                    const auto&    hair      = mHairModelSystem->getHairGeometry(hairIndex);

                    spdlog::info("Loaded Hair model: {} [v={}, S={}, s={}]", hair.name, hair.vertexCount, hair.strandCount, hair.strandletCount);
                }
                catch (const std::runtime_error& ex)
                {
                    spdlog::error(ex.what());
                }
            }

            mHairModelSystem->createBuffers();
        }

        mClassicRenderer = makeUnique<ClassicHairRenderer>(mRHI, mHairModelSystem.get());

        mHybrid = makeUnique<HybridHairRenderer>(mRHI, mHairModelSystem.get());
        mUserInterface->addComponent<HybridHairRendererUI>(mHybrid.get());
    }

    void HairView::onEvent(const SDL_Event& event)
    {
        mCameraSystem->onEvent(event);
    }

    void HairView::onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        mCameraSystem->onUpdate(frameData);
    }

    void HairView::onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        IHairRenderer* renderer = mHybrid.get();
        if (!mUseHybrid)
        {
            renderer = mClassicRenderer.get();
        }

        renderer->execute(pCommandList, frameData, mCameraSystem->getBuffer(frameData.currentFrame)->getAddress());

        auto* pFinalImage = renderer->getResult(frameData.currentFrame).get();
        pCommandList->blitToSwapchain(pFinalImage, mRHI->getSwapchain(), frameData.acquiredIndex);
    }

    void HairView::onDrawUI()
    {
        ImGui::Begin("Hair Renderer");
        ImGui::Checkbox("Use Hybrid Renderer", &mUseHybrid);
        ImGui::End();
    }
}
