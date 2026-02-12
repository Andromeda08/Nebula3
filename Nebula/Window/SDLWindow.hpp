#pragma once

#include <SDL3/SDL.h>

#include "WindowCreateInfo.hpp"
#include "Core/Macro.hpp"
#include "RHI/RHI.hpp"
#include "../RHI/IWindow.hpp"

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

    RHI::Extent2D getFramebufferSize() const override;

    #ifdef nbl_VulkanRHI
    std::vector<const char*> getVulkanInstanceExtensions() const override;

    void createVulkanSurface(const vk::Instance& instance, vk::SurfaceKHR* pSurface) const override;
    #endif

    #ifdef nbl_MetalRHI
    CA::MetalLayer* getMetalLayer() const override;
    #endif

private:
    float       mDisplayScaling  = 1.0f;
    Size2D      mWindowSize      = {};
    Size2D      mFramebufferSize = {};
    std::string mTitle           = "Nebula3 Window";

    #ifdef nbl_MetalRHI
    SDL_MetalView mMetalView = nullptr;
    #endif

    SDL_Window* mWindow = nullptr;
};
