#pragma once

#include "Types.hpp"
#include "Math/DeltaTime.hpp"
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
    UPtr<Window>            mWindow;
    UPtr<RHI::VulkanRHI>    mVulkanRHI;
    DeltaTime               mDeltaTime;
};

extern App* gApplication;
