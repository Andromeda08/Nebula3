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
#include "Hair/HairGeometry.hpp"
#include "Hair/Render/HairRenderer.hpp"
#include "Hair/Render/Complex/SoftwareRasterizer.hpp"
#include "Level/Level.hpp"
#include "Level/Render/LevelRenderer.hpp"

class App
{
public:
    nbl_DISABLE_COPY(App);

    App();
    static UPtr<App> create() noexcept;

    void run();

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

    UPtr<nbl::Level>             mLevel;
    UPtr<nbl::LevelRenderer>     mLevelRenderer;

    UPtr<nbl::HairModelSystem>   mHairModelSystem;
    UPtr<nbl::HairRenderer>      mHairRenderer;

    UPtr<nbl::SoftwareRasterizer> mSoftwareRasterizer;

    UPtr<TitleScreen>            mTitleScreen;
};

extern App* gApplication;
