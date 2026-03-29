#include "SDLWindow.hpp"

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include "Core/Util.hpp"
#include "Input/DualSense.hpp"

SDLWindow::SDLWindow(const WindowCreateInfo& createInfo)
{
    DualSense::addGamepadMapping();

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD))
    {
        exitWithError("Failed to initialize windowing system.");
    }

    uint64_t windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN;
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

    findGamepad();
}

SDLWindow::~SDLWindow()
{
    if (mGamepad)
    {
        if (SDL_GetGamepadType(mGamepad) == SDL_GAMEPAD_TYPE_PS5)
        {
            SDL_SetGamepadLED(mGamepad, 0, 0, 255);
        }
        SDL_CloseGamepad(mGamepad);
    }
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

void SDLWindow::useGamepad(const SDL_JoystickID& joystickId) noexcept
{
    const char*  name    = SDL_GetGamepadNameForID(joystickId);
    const Uint16 vendor  = SDL_GetGamepadVendorForID(joystickId);
    const Uint16 product = SDL_GetGamepadProductForID(joystickId);

    mGamepad = SDL_OpenGamepad(joystickId);
    spdlog::info("Gamepad connected: {} (Vendor: 0x{:04X}  Product: 0x{:04X})", name, vendor, product);

    if (const auto type = SDL_GetGamepadType(mGamepad); type == SDL_GAMEPAD_TYPE_PS5)
    {
        //SDL_SetGamepadLED(mGamepad, 136, 57, 239);
        SDL_SetGamepadLED(mGamepad, 36, 102, 94);
    }
}

void SDLWindow::removeGamepad(const SDL_JoystickID& joystickId) noexcept
{
    if (!mGamepad)
    {
        return;
    }
    if (joystickId != SDL_GetGamepadID(mGamepad))
    {
        return;
    }

    spdlog::info("Gamepad disconnected: {}", SDL_GetGamepadName(mGamepad));
    SDL_CloseGamepad(mGamepad);
}

SDL_Gamepad* SDLWindow::getGamepad() const noexcept
{
    return mGamepad;
}

vk::Extent2D SDLWindow::getFramebufferSize() const
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
        exitWithError("Failed to create Vulkan Surface");
    }
}

void SDLWindow::findGamepad() noexcept
{
    int32_t nJoysticks = 0;
    if (SDL_JoystickID* joysticks = SDL_GetJoysticks(&nJoysticks))
    {
        for (int32_t i = 0; i < nJoysticks; ++i)
        {
            if (SDL_IsGamepad(joysticks[i]))
            {
                useGamepad(joysticks[i]);
            }
        }

        SDL_free(joysticks);
    }
}
