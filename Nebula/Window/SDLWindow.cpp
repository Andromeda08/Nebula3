#include "SDLWindow.hpp"

#include <metal/metal.hpp>
#include <SDL3/SDL_metal.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>


SDLWindow::SDLWindow(const WindowCreateInfo& createInfo)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        // exitWithError("Failed to initialize windowing system.");
        __builtin_debugtrap();
        exit(EXIT_FAILURE);
    }

    uint64_t windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (createInfo.isResizable)
    {
        windowFlags |= SDL_WINDOW_RESIZABLE;
    }

    const auto width  = static_cast<int32_t>(createInfo.size.width);
    const auto height = static_cast<int32_t>(createInfo.size.height);
    mWindow = SDL_CreateWindow(createInfo.title.data(), width, height, windowFlags);

    mDisplayScaling = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());;

    int32_t w, h;
    SDL_GetWindowSize(mWindow, &w, &h);
    mWindowSize = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };

    SDL_GetWindowSizeInPixels(mWindow, &w, &h);
    mFramebufferSize = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };

}

SDLWindow::~SDLWindow()
{
    if (mWindow)
    {
        SDL_DestroyWindow(mWindow);
    }
    SDL_Quit();
}

void SDLWindow::update() noexcept
{
    int32_t w, h;
    SDL_GetWindowSize(mWindow, &w, &h);
    mWindowSize = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };

    SDL_GetWindowSizeInPixels(mWindow, &w, &h);
    mFramebufferSize = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
}

SDL_Window* SDLWindow::getHandle() const noexcept
{
    return mWindow;
}

RHI::Extent2D SDLWindow::getFramebufferSize() const
{
    return { mFramebufferSize.width, mFramebufferSize.height };
}

std::vector<const char*> SDLWindow::getVulkanInstanceExtensions() const
{
    uint32_t extensionCount;
    const auto extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    return {extensions, extensions + extensionCount};
}

void SDLWindow::createVulkanSurface(const vk::Instance& instance, vk::SurfaceKHR* pSurface) const
{
    if (!SDL_Vulkan_CreateSurface(mWindow, instance, nullptr, reinterpret_cast<VkSurfaceKHR*>(pSurface)))
    {
        __builtin_debugtrap();
        exit(EXIT_FAILURE);
    }
}

CA::MetalLayer* SDLWindow::getMetalLayer() const
{
    const SDL_MetalView view = SDL_Metal_CreateView(mWindow);
    auto* pLayer = static_cast<CA::MetalLayer*>(SDL_Metal_GetLayer(view));
    return pLayer;
}
