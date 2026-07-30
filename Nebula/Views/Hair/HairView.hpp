#pragma once

#include "Core/View.hpp"
#include "Hair/HairGeometry.hpp"
#include "Hair/Render/ClassicHairRenderer.hpp"
#include "Hair/Render/Complex/HybridHairRenderer.hpp"
#include "Level/Camera/CameraSystem.hpp"
#include "Level/Light/LightSystem.hpp"
#include "Level/Render/TonemapPass.hpp"

namespace nbl
{
    class HairView : public View
    {
    public:
        HairView(nbl_ViewCtorParams);

        ~HairView() override = default;

        void onEvent(const SDL_Event& event) override;

        void onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override;

        void onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override;

        void onDrawUI() override;

    private:
        IHairRenderer* getRenderer() const;

        bool                        mUseHybrid = true;

        UPtr<CameraSystem>          mCameraSystem;
        UPtr<LightSystem>           mLightSystem;
        UPtr<HairModelSystem>       mHairModelSystem;
        UPtr<HybridHairRenderer>    mHybrid;
        UPtr<ClassicHairRenderer>   mClassicRenderer;
        UPtr<TonemapPass>           mTonemapPass;
    };
}
