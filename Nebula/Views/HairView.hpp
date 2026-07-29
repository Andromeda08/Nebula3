#pragma once

#include "Core/View.hpp"
#include "Hair/CyLoader.hpp"
#include "Hair/HairGeometry.hpp"
#include "Hair/Render/Complex/HybridHairRenderer.hpp"
#include "Hair/Render/ClassicHairRenderer.hpp"
#include "Level/Camera/CameraSystem.hpp"
#include "Level/Camera/OrbitCamera.hpp"

namespace nbl
{
    class HairView : public View
    {
    public:
        HairView(nbl_ViewCtorParams)
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

        ~HairView() override = default;

        void onEvent(const SDL_Event& event) override
        {
            mCameraSystem->onEvent(event);
        }

        void onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override
        {
            mCameraSystem->onUpdate(frameData);
        }

        void onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override
        {
            if (mUseHybrid)
            {
                mHybrid->execute(pCommandList, frameData, mCameraSystem->getBuffer(frameData.currentFrame)->getAddress());
            }
            else
            {
                mClassicRenderer->render(pCommandList, frameData, 0, mCameraSystem->getBuffer(frameData.currentFrame)->getAddress());
            }

            auto* pFinalImage = mUseHybrid ? mHybrid->getResult(frameData.currentFrame).get() : mClassicRenderer->getResult(frameData.currentFrame).get();
            pCommandList->blitToSwapchain(pFinalImage, mRHI->getSwapchain(), frameData.acquiredIndex);
        }

        void onDrawUI() override
        {
            ImGui::Begin("Hair Renderer");
            ImGui::Checkbox("Use Hybrid Renderer", &mUseHybrid);
            ImGui::End();
        }

    private:
        bool                      mUseHybrid = true;

        UPtr<CameraSystem>        mCameraSystem;
        UPtr<HairModelSystem>     mHairModelSystem;
        UPtr<HybridHairRenderer>  mHybrid;
        UPtr<ClassicHairRenderer> mClassicRenderer;
    };
}
