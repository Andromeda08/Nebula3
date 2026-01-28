#pragma once

#include "Types.hpp"
#include "Math/DeltaTime.hpp"
#include "RenderGraph/RenderGraphContext.hpp"
#include "Scene/Scene.hpp"
#include "UserInterface/UserInterface.hpp"
#include "VulkanRHI/VulkanRHI.hpp"
#include "Window/SDLWindow.hpp"

class App
{
public:
    nbl_DISABLE_COPY(App);

    App();
    static UPtr<App> create() noexcept;

    void run_renderPathLoop();

private:
    bool                         mRunning = false;

    DeltaTime                    mDeltaTime;
    SPtr<SDLWindow>              mWindow;
    SPtr<RHI::VulkanRHI>         mVulkanRHI;
    UPtr<UserInterface>          mUserInterface;
    SPtr<rg::RenderGraphContext> mRenderGraphContext;
    UPtr<Scene>                  mScene;
};

extern App* gApplication;
