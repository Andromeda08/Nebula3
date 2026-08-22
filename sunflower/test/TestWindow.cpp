#include "TestWindow.hpp"

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

TestWindow::TestWindow(const TestWindowCreateInfo& createInfo)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        ::sunflower::exit("Failed to initialize windowing system.");
    }

    static constexpr uint64_t sWindowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    const auto width  = static_cast<int32_t>(createInfo.size.width);
    const auto height = static_cast<int32_t>(createInfo.size.height);
    mWindow = SDL_CreateWindow(createInfo.title.data(), width, height, sWindowFlags);

    mDisplayScaling = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());;

    int32_t w, h;
    SDL_GetWindowSize(mWindow, &w, &h);
    mWindowSize = {
        .width  = static_cast<uint32_t>(w),
        .height = static_cast<uint32_t>(h),
        .depth  = 1
    };

    SDL_GetWindowSizeInPixels(mWindow, &w, &h);
    mFramebufferSize = {
        .width  = static_cast<uint32_t>(w),
        .height = static_cast<uint32_t>(h),
        .depth  = 1
    };
}

TestWindow::~TestWindow()
{
    if (mWindow)
    {
        SDL_DestroyWindow(mWindow);
    }
    SDL_Quit();
}

SDL_Window* TestWindow::getHandle() const noexcept
{
    return mWindow;
}

sunflower::Size TestWindow::getFramebufferSize()
{
    return mFramebufferSize;
}

std::vector<const char*> TestWindow::getVulkanInstanceExtensions()
{
    uint32_t extensionCount;
    const auto extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    return {extensions, extensions + extensionCount};
}

void TestWindow::createVulkanSurface(const vk::Instance& instance, vk::SurfaceKHR* pSurface)
{
    if (!SDL_Vulkan_CreateSurface(mWindow, instance, nullptr, reinterpret_cast<VkSurfaceKHR*>(pSurface)))
    {
        ::sunflower::exit("Failed to create Vulkan Surface");
    }
}
