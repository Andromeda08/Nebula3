#pragma once

#include "Core/View.hpp"

#include "Interface/FadeEffect.hpp"
#include "Interface/Interface.hpp"

namespace nbl
{
    class UITestView : public View
    {
    public:
        UITestView(nbl_ViewCtorParams)
        : nbl_ViewBaseCtor
        {
            mName = "UITest";

            InterfaceParams ip = {
                { 1920.0f, 1080.0f },
                { 64.0f, 64.0f },
                mRHI,
                mTextureManager,
                {},
                true
            };
            mInterface = makeUnique<Interface>(ip);
            mFadeEffect = makeUnique<FadeEffect>(mRHI);
        }

        ~UITestView() override = default;

        void onEvent(const SDL_Event& event) override {}

        void onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override
        {
            static bool started = false;
            if (!started)
            {
                mFadeEffect->trigger();
                started = true;
            }
        }

        void onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override
        {
            mInterface->render(pCommandList, frameData);
            mFadeEffect->execute(mInterface->getResult(frameData.currentFrame), pCommandList, frameData);
            pCommandList->blitToSwapchain(mFadeEffect->getResult(frameData.currentFrame).get(), mRHI->getSwapchain(), frameData.acquiredIndex);
        }

    private:
        UPtr<Interface>     mInterface;
        UPtr<FadeEffect>    mFadeEffect;
    };
}
