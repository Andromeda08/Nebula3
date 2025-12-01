#pragma once

#include "Types.hpp"
#include "Math/DeltaTime.hpp"
#include "RenderGraph/RenderGraphContext.hpp"
#include "UserInterface/UserInterface.hpp"
#include "VulkanRHI/VulkanRHI.hpp"
#include "Window/Window.hpp"

class App
{
public:
    nbl_DISABLE_COPY(App);

    App();
    static UPtr<App> create() noexcept;

    void run();

    void run_renderPathLoop();

private:
    DeltaTime                    mDeltaTime;
    SPtr<Window>                 mWindow;
    SPtr<RHI::VulkanRHI>         mVulkanRHI;
    UPtr<UserInterface>          mUserInterface;
    SPtr<rg::RenderGraphContext> mRenderGraphContext;
};

extern App* gApplication;
