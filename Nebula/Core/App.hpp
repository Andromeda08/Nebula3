#pragma once

#include "Types.hpp"
#include "Input/Gamepad.hpp"
#include "Math/DeltaTime.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneV2.hpp"
#include "UserInterface/UserInterface.hpp"
#include "VulkanRHI/VulkanRHI.hpp"
#include "Window/SDLWindow.hpp"

#include "Game/TitleScreen.hpp"
#include "Level/Level.hpp"
#include "Level/Render/LevelRenderer.hpp"

class App
{
public:
    nbl_DISABLE_COPY(App);

    App();
    static UPtr<App> create() noexcept;

    void run_renderPathLoop();

    ~App();

private:
    bool                         mRunning = false;
    float                        mCPUFramerate = 0.0f;

    DeltaTime                    mDeltaTime;
    SPtr<SDLWindow>              mWindow;
    UPtr<GamepadManager>         mGamepadManager;

    SPtr<RHI::VulkanRHI>         mVulkanRHI;
    UPtr<TextureManager>         mTextureManager;

    UPtr<UserInterface>          mUserInterface;
    // SPtr<rg::RenderGraphContext> mRenderGraphContext;
    UPtr<SceneV2>                mScene;

    UPtr<nbl::Level>             mLevel;
    UPtr<nbl::LevelRenderer>     mLevelRenderer;

    UPtr<TitleScreen>            mTitleScreen;
};

extern App* gApplication;
