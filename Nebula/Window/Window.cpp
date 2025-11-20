#include "Window.hpp"

#include <print>
#include <vulkan/vulkan.hpp>

#include "Core/ToString.hpp"

Window::Window(const WindowCreateInfo& createInfo)
: mTitle(createInfo.title)
{
    assert(glfwInit());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, false);

    GLFWmonitor*       display   = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(display);

    assert(createInfo.size.width != 0 && createInfo.size.height != 0);
    const int32_t width = std::min(static_cast<int32_t>(createInfo.size.width), videoMode->width);
    const int32_t height = std::min(static_cast<int32_t>(createInfo.size.height), videoMode->height);

    mWindow = glfwCreateWindow(width, height, mTitle.c_str(), nullptr, nullptr);
    assert(mWindow);

    int32_t w, h;
    glfwGetWindowSize(mWindow, &w, &h);
    mWindowSize = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };

    glfwGetFramebufferSize(mWindow, &w, &h);
    mFramebufferSize = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };

    glfwSetKeyCallback(mWindow, Window::defaultKeyHandler);
    glfwSetWindowUserPointer(mWindow, this);

    std::println("[Info] Created Window ({})\n\t- Window Size: {}\n\t- Framebuffer Size: {}",
        mTitle, toString(mWindowSize), toString(mFramebufferSize));
}

Window::~Window()
{
    if (mWindow)
    {
        glfwDestroyWindow(mWindow);
    }
    glfwTerminate();
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(mWindow);
}

void Window::pollEvents() const
{
    glfwPollEvents();
}

vk::Extent2D Window::getFramebufferSize() const
{
    return { mFramebufferSize.width, mFramebufferSize.height };
}

std::vector<const char*> Window::getVulkanInstanceExtensions() const
{
    uint32_t extensionCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    return {extensions, extensions + extensionCount};
}

void Window::createVulkanSurface(const vk::Instance& instance, vk::SurfaceKHR* pSurface) const
{
    const VkResult result = glfwCreateWindowSurface(instance, mWindow, nullptr, reinterpret_cast<VkSurfaceKHR*>(pSurface));
    assert(result == VK_SUCCESS);
}

void Window::defaultKeyHandler(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}
