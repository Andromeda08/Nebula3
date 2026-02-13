#pragma once

#include <SDL3/SDL.h>

#include <VulkanRHI/IWindow.hpp>
#include "WindowCreateInfo.hpp"
#include "Core/Macro.hpp"

class SDLWindow : public RHI::IWindow
{
public:
    nbl_DISABLE_COPY(SDLWindow);

    explicit SDLWindow(const WindowCreateInfo& createInfo);

    [[nodiscard]] static UPtr<SDLWindow> create(const WindowCreateInfo& createInfo) noexcept
    {
        return makeUnique<SDLWindow>(createInfo);
    }

    ~SDLWindow() override;

    // Re-fetch window and framebuffer size.
    void update() noexcept;

    [[nodiscard]] SDL_Window* getHandle() const noexcept;

    // RHI Interface Implementation

    vk::Extent2D getFramebufferSize() const override;

    std::vector<const char*> getVulkanInstanceExtensions() const override;

    void createVulkanSurface(const vk::Instance& instance, vk::SurfaceKHR* pSurface) const override;

private:
    float       mDisplayScaling  = 1.0f;
    Size2D      mWindowSize      = {};
    Size2D      mFramebufferSize = {};
    SDL_Window* mWindow          = nullptr;
    std::string mTitle           = "Nebula3 Window";
};
