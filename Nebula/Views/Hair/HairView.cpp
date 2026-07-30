#include "HairView.hpp"

#include "Hair/CyLoader.hpp"
#include "Level/Camera/FlyingCamera.hpp"
#include "Level/Camera/OrbitCamera.hpp"

namespace nbl
{
    HairView::HairView(nbl_ViewCtorParams)
    : nbl_ViewBaseCtor
    {
        mName = "HairView";

        mCameraSystem = makeUnique<CameraSystem>(mRHI);
        mCameraSystem->addCamera<OrbitCamera>(true, 50.0f);

        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        mCameraSystem->addCamera<FlyingCamera>(false, glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));

        mLightSystem = makeUnique<LightSystem>(mRHI);
        const auto hInitLight = mLightSystem->addLight({
            .vector    = { 0.2f, 0.8f, 0.5f },
            .intensity = 1.0f,
            .type      = LightType::Directional,
        });

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
        mHybrid          = makeUnique<HybridHairRenderer>(mRHI, mHairModelSystem.get());

        mTonemapPass     = TonemapPass::create({
            .outputFormat = mHybrid->getResult(0)->getProperties().format,
            .rhi = mRHI,
        });
    }

    void HairView::onEvent(const SDL_Event& event)
    {
        mCameraSystem->onEvent(event);
    }

    void HairView::onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        mCameraSystem->onUpdate(frameData);
        mLightSystem->onUpdate(pCommandList);
    }

    void HairView::onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        auto* renderer = getRenderer();

        renderer->execute(pCommandList, frameData, {
            .cameraBuffer = mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
            .lightsBuffer = mLightSystem->getBuffer()->getAddress(),
        });

        const auto& tonemapInput = renderer->getResult(frameData.currentFrame);
        mTonemapPass->execute(tonemapInput, pCommandList, frameData);

        auto* pFinalImage = mTonemapPass->getResult(frameData.currentFrame).get();
        pCommandList->blitToSwapchain(pFinalImage, mRHI->getSwapchain(), frameData.acquiredIndex);
    }

    void HairView::onDrawUI()
    {
        ImGui::Begin("Hair Renderer");

        ImGui::Checkbox("Use Hybrid Renderer", &mUseHybrid);

        mCameraSystem->onDrawUI(true);
        mLightSystem->onDrawUI();

        ImGui::End();

        if (auto* renderer = getRenderer())
        {
            renderer->onDrawUI();
        }
    }

    IHairRenderer* HairView::getRenderer() const
    {
        IHairRenderer* renderer = mHybrid.get();
        if (!mUseHybrid)
        {
            renderer = mClassicRenderer.get();
        }
        return renderer;
    }
}
