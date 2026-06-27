#pragma once

#include "Core/View.hpp"

namespace nbl
{
    class FractalView : public View
    {
    public:
        FractalView(nbl_ViewCtorParams);

        ~FractalView() override = default;

        void onEvent(const SDL_Event& event) override {}

        void onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override {}

        void onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override {}

    private:
        PerFrameArray<SPtr<RHI::Image>> mTargets;
        UPtr<RHI::GraphicsPipeline2>    mPipeline;
    };
}
