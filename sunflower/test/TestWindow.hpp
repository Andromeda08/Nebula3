#pragma once

#include <SDL3/SDL.h>
#include <rhi/Common.hpp>
#include <rhi/IWindow.hpp>

struct TestWindowCreateInfo
{
    std::string     title;
    sunflower::Size size;
};

class TestWindow : public sunflower::rhi::IWindow
{
public:
    sunflower_DisableCopy(TestWindow);
    sunflower_Create(TestWindow, std::unique_ptr);

    ~TestWindow() override;

    [[nodiscard]] SDL_Window* getHandle() const noexcept;

    [[nodiscard]] sunflower::Size getFramebufferSize() override;

    std::vector<const char*> getVulkanInstanceExtensions() override;

    void createVulkanSurface(const vk::Instance& instance, vk::SurfaceKHR* pSurface) override;

private:
    SDL_Window*     mWindow          = nullptr;

    float           mDisplayScaling  = 1.0f;
    sunflower::Size mWindowSize      = {};
    sunflower::Size mFramebufferSize = {};
    std::string     mTitle           = "Sunflower Test";
};
