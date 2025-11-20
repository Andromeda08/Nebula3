#pragma once

#include <GLFW/glfw3.h>

#include "Core/Macro.hpp"
#include "Core/Types.hpp"
#include "VulkanRHI/IWindow.hpp"

struct WindowCreateInfo
{
    Size2D      size;
    std::string title;
};

class Window final : public RHI::IWindow
{
public:
    nbl_DISABLE_COPY(Window);
    nbl_CTOR(Window);

    ~Window() override;

    bool shouldClose() const;

    void pollEvents() const;

    [[nodiscard]] GLFWwindow* getHandle() const noexcept
    {
        return mWindow;
    }

    Size2D getWindowSize() const noexcept
    {
        return mWindowSize;
    }

    #pragma region "VulkanRHI Interface Implementation"

    vk::Extent2D getFramebufferSize() const override;
    std::vector<const char*> getVulkanInstanceExtensions() const override;
    void createVulkanSurface(const vk::Instance& instance, vk::SurfaceKHR* pSurface) const override;

    #pragma endregion

private:
    static void defaultKeyHandler(GLFWwindow* window, int key, int scancode, int action, int mods);

    Size2D              mWindowSize;
    Size2D              mFramebufferSize;
    const std::string   mTitle;
    GLFWwindow*         mWindow {nullptr};
};
