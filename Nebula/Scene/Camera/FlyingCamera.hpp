#pragma once

#include <glm/glm.hpp>
#include <SDL3/SDL_events.h>
#include "ICamera.hpp"

class FlyingCamera final : public ICamera
{
public:
    FlyingCamera(glm::ivec2 size, glm::vec3 eye, float h_fov = 75.0f, float near = 0.1f, float far = 10000.0f);

    ~FlyingCamera() override;

    void onEvent(const SDL_Event& event) noexcept override;

    void onUpdate() noexcept override;

    const glm::vec3& eye() const override;

    glm::mat4 view() const override;

    glm::mat4 projection() const override;

    CameraData getCameraData() const override;

private:
    void handleKeyEvent(const SDL_KeyboardEvent& keyEvent) noexcept;

    void handleMouseEvent(const SDL_MouseMotionEvent& motionEvent) noexcept;

    glm::ivec2 mSize;
    glm::vec3  mEye;
    glm::vec3  mUp = {0, 1, 0};
    glm::vec3  mOrientation = {0, 0, -1};

    float mNear;
    float mFar;
    float mFov;

    float mSpeed {0.25f};
    float mSensitivity {50.0f};

    bool mMouseCaptured = false;

    struct KeyState {
        bool forward  = false;
        bool backward = false;
        bool left     = false;
        bool right    = false;
        bool up       = false;
        bool down     = false;
    } mKeyState;
};