#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Scene/Types/CameraData.hpp"

class ICamera
{
public:
    ICamera() = default;
    virtual ~ICamera() = default;

    virtual const glm::vec3& eye()           const = 0;
    virtual       glm::mat4  view()          const = 0;
    virtual       glm::mat4  projection()    const = 0;
    virtual       CameraData getCameraData() const = 0;

    virtual void registerKeys(GLFWwindow* pWindow) {}
    virtual void registerMouse(GLFWwindow* pWindow) {}
};
