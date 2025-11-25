#pragma once

#include "Types.hpp"
#include "Math/DeltaTime.hpp"
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

private:
    SPtr<Window>            mWindow;
    SPtr<RHI::VulkanRHI>    mVulkanRHI;
    UPtr<UserInterface>     mUserInterface;
    DeltaTime               mDeltaTime;
};

extern App* gApplication;
