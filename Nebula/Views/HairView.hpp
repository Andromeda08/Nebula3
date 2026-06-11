#pragma once

#include "Core/View.hpp"
#include "Hair/CyLoader.hpp"
#include "Hair/HairGeometry.hpp"
#include "Hair/Render/Complex/HybridHairRenderer.hpp"

namespace nbl
{
    class HairView : public View
    {
    public:
        HairView(nbl_ViewCtorParams)
        : nbl_ViewBaseCtor
        {
            mName = "HairView";

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

            mHybrid = makeUnique<HybridHairRenderer>(mRHI, mHairModelSystem.get());
            mUserInterface->addComponent<HybridHairRendererUI>(mHybrid.get());
        }

        ~HairView() override = default;

        void onEvent(const SDL_Event& event) override
        {
        }

        void onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override
        {
        }

        void onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override
        {
            // TODO: Local Camera
            // mHybrid->execute(pCommandList, frameData, mLevel->getCameraBuffer(frameData.currentFrame));
        }

    private:
        UPtr<HairModelSystem>    mHairModelSystem;
        UPtr<HybridHairRenderer> mHybrid;
    };
}
