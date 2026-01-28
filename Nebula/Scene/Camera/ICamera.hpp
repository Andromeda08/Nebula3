#pragma once

#include <glm/glm.hpp>
#include <SDL3/SDL_events.h>
#include "Scene/Types/CameraData.hpp"

class ICamera
{
public:
    ICamera() = default;
    virtual ~ICamera() = default;

    virtual void onEvent(const SDL_Event& event) noexcept {}
    virtual void onUpdate() noexcept {}

    virtual const glm::vec3& eye()           const = 0;
    virtual       glm::mat4  view()          const = 0;
    virtual       glm::mat4  projection()    const = 0;
    virtual       CameraData getCameraData() const = 0;
};
