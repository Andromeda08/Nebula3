#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <SDL3/SDL_mouse.h>
#include "ICamera.hpp"

class FlyingCamera final : public ICamera
{
public:
    FlyingCamera(glm::ivec2 size, glm::vec3 eye, float h_fov = 75.0f, float near = 0.1f, float far = 10000.0f)
    : ICamera(), mSize(size), mEye(eye), mNear(near), mFar(far), mFov(h_fov) {}

    ~FlyingCamera() override = default;

    void onEvent(const SDL_Event& event) noexcept override
    {
        switch (event.type)
        {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                handleKeyEvent(event.key);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    mMouseCaptured = true;
                    SDL_SetWindowRelativeMouseMode(SDL_GetWindowFromID(event.button.windowID), true);
                }
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    mMouseCaptured = false;
                    SDL_SetWindowRelativeMouseMode(SDL_GetWindowFromID(event.button.windowID), false);
                }
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                if (mMouseCaptured)
                {
                    handleMouseEvent(event.motion);
                }
                break;
            }
            default:
                break;
        }
    }

    void onUpdate() noexcept override
    {
        const glm::vec3 right = glm::normalize(glm::cross(mOrientation, mUp));

        if (mKeyState.forward)
            mEye += mSpeed * mOrientation;
        if (mKeyState.backward)
            mEye -= mSpeed * mOrientation;
        if (mKeyState.left)
            mEye -= mSpeed * right;
        if (mKeyState.right)
            mEye += mSpeed * right;
        if (mKeyState.up)
            mEye += (mSpeed / 2.0f) * mUp;
        if (mKeyState.down)
            mEye -= (mSpeed / 2.0f) * mUp;
    }

    const glm::vec3& eye() const override { return mEye; }

    glm::mat4 view() const override
    {
        return glm::lookAt(mEye, mEye + mOrientation, mUp);
    }

    glm::mat4 projection() const override
    {
        return glm::perspective(
            glm::radians(mFov),
            static_cast<float>(mSize.x) / static_cast<float>(mSize.y),
            mNear, mFar);
    }

    CameraData getCameraData() const override
    {
        auto v = view();
        auto p = projection();
        auto e = eye();

        return {
            .view = v,
            .proj = p,
            .viewInverse = glm::inverse(v),
            .projInverse = glm::inverse(p),
            .eye = { e.x, e.y, e.z, 1.0f },
            .nearPlane = mNear,
            .farPlane = mFar,
        };
    }

private:
    void handleKeyEvent(const SDL_KeyboardEvent& keyEvent) noexcept
    {
        const bool pressed = (keyEvent.type == SDL_EVENT_KEY_DOWN);

        switch (keyEvent.scancode)
        {
            case SDL_SCANCODE_W:
                mKeyState.forward = pressed;
                break;
            case SDL_SCANCODE_S:
                mKeyState.backward = pressed;
                break;
            case SDL_SCANCODE_A:
                mKeyState.left = pressed;
                break;
            case SDL_SCANCODE_D:
                mKeyState.right = pressed;
                break;
            case SDL_SCANCODE_SPACE:
                mKeyState.up = pressed;
                break;
            case SDL_SCANCODE_LCTRL:
                mKeyState.down = pressed;
                break;
            default:
                break;
        }
    }

    void handleMouseEvent(const SDL_MouseMotionEvent& motionEvent) noexcept
    {
        const float rotX = mSensitivity * motionEvent.yrel / mSize.y;
        const float rotY = mSensitivity * motionEvent.xrel / mSize.x;

        const glm::vec3 newOrientation = glm::rotate(
            mOrientation,
            glm::radians(-rotX),
            glm::normalize(glm::cross(mOrientation, mUp))
        );

        if (glm::abs(glm::angle(newOrientation, mUp) - glm::radians(90.0f)) <= glm::radians(85.0f))
        {
            mOrientation = newOrientation;
        }

        mOrientation = glm::rotate(mOrientation, glm::radians(-rotY), mUp);
    }

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