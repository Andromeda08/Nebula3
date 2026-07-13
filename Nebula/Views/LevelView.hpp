#pragma once

#include "Core/View.hpp"
#include "Level/Level.hpp"
#include "Level/Render/LevelRenderer.hpp"

namespace nbl
{
    class LevelView : public View
    {
    public:
        LevelView(nbl_ViewCtorParams)
        : nbl_ViewBaseCtor
        {
            mName = "LevelView";
            mLevel = makeUnique<Level>(mRHI, mUserInterface, mTextureManager);
            mLevelRenderer = makeUnique<LevelRenderer>(mRHI, mTextureManager, mLevel.get());
        }

        ~LevelView() override = default;

        void onEvent(const SDL_Event& event) override
        {
            mLevel->onEvent(event);
        }

        void onUpdate(const float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override
        {
            mLevel->onUpdate(dt, frameData, pCommandList);
        }

        void onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override
        {
            mLevelRenderer->render(frameData, pCommandList);
        }

    private:
        UPtr<Level>           mLevel;
        UPtr<LevelRenderer>   mLevelRenderer;
    };
}
