#include "FlyingCamera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

FlyingCamera::FlyingCamera(glm::ivec2 size, glm::vec3 eye, float h_fov, float near, float far)
: ICamera()
, mSize(size)
, mEye(eye)
, mNear(near)
, mFar(far)
, mFov(h_fov)
{
}

FlyingCamera::~FlyingCamera() = default;

void FlyingCamera::onEvent(const SDL_Event& event) noexcept
{
    auto* pWindow = SDL_GetWindowFromID(event.button.windowID);
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
                SDL_SetWindowRelativeMouseMode(pWindow, true);
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                mMouseCaptured = false;
                mInputState = {};
                SDL_SetWindowRelativeMouseMode(pWindow, false);
                SDL_WarpMouseInWindow(pWindow, mSize.x / 2.0f, mSize.y / 2.0f);
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
        case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
            handleGamepadAxisEvent(event.gaxis);
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP: {
            handleGamepadButtonEvent(event.gbutton);
            break;
        }
        case SDL_EVENT_WINDOW_FOCUS_LOST: {
            mInputState = {};
            mMouseCaptured = false;
            SDL_SetWindowRelativeMouseMode(pWindow, false);
            break;
        }
        default:
            break;
    }
}

void FlyingCamera::onUpdate() noexcept
{
    const glm::vec3 right = glm::normalize(glm::cross(mOrientation, mUp));

    if (mInputState.forward)
    {
        mEye += mSpeed * mOrientation;
    }
    if (mInputState.backward)
    {
        mEye -= mSpeed * mOrientation;
    }
    if (mInputState.left)
    {
        mEye -= mSpeed * right;
    }
    if (mInputState.right)
    {
        mEye += mSpeed * right;
    }
    if (mInputState.up)
    {
        mEye += (mSpeed / 2.0f) * mUp;
    }
    if (mInputState.down)
    {
        mEye -= (mSpeed / 2.0f) * mUp;
    }
    
    if (glm::abs(mInputState.leftX) > mDeadzone)
    {
        mEye += (mSpeed * 0.5f) * mInputState.leftX * right;
    }
    if (glm::abs(mInputState.leftY) > mDeadzone)
    {
        mEye -= (mSpeed * 0.5f) * mInputState.leftY * mOrientation;
    }

    if (glm::abs(mInputState.rightX) > mDeadzone || glm::abs(mInputState.rightY) > mDeadzone)
    {
        SDL_MouseMotionEvent fakeMotion {};
        fakeMotion.xrel = mInputState.rightX * mSize.x * 0.05f;
        fakeMotion.yrel = mInputState.rightY * mSize.y * 0.05f;
        handleMouseEvent(fakeMotion);
    }
}

const glm::vec3& FlyingCamera::eye() const
{
    return mEye;
}

glm::mat4 FlyingCamera::view() const
{
    return glm::lookAt(mEye, mEye + mOrientation, mUp);
}

glm::mat4 FlyingCamera::projection() const
{
    auto proj = glm::perspective(
        glm::radians(mFov),
        static_cast<float>(mSize.x) / static_cast<float>(mSize.y),
        mNear, mFar);
    proj[1][1] *= -1.0f;
    return proj;
}

CameraData FlyingCamera::getCameraData() const
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

void FlyingCamera::handleKeyEvent(const SDL_KeyboardEvent& keyEvent) noexcept
{
    const bool pressed = (keyEvent.type == SDL_EVENT_KEY_DOWN);

    switch (keyEvent.scancode)
    {
        case SDL_SCANCODE_W: {
            mInputState.forward = pressed;
            break;
        }
        case SDL_SCANCODE_S: {
            mInputState.backward = pressed;
            break;
        }
        case SDL_SCANCODE_A: {
            mInputState.left = pressed;
            break;
        }
        case SDL_SCANCODE_D: {
            mInputState.right = pressed;
            break;
        }
        case SDL_SCANCODE_SPACE: {
            mInputState.up = pressed;
            break;
        }
        case SDL_SCANCODE_LCTRL: {
            mInputState.down = pressed;
            break;
        }
        default: {
            break;
        }
    }
}

void FlyingCamera::handleMouseEvent(const SDL_MouseMotionEvent& motionEvent) noexcept
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

void FlyingCamera::handleGamepadButtonEvent(const SDL_GamepadButtonEvent& buttonEvent) noexcept
{
    const bool pressed = (buttonEvent.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);

    switch (buttonEvent.button)
    {
        case 9: {
            mInputState.down = pressed;
            break;
        }
        case 10: {
            mInputState.up = pressed;
            break;
        }
        default: {
            break;
        }
    }
}

void FlyingCamera::handleGamepadAxisEvent(const SDL_GamepadAxisEvent& axisEvent) noexcept
{
    const float value = axisEvent.value / 32767.0f;
    switch (axisEvent.axis)
    {
        case SDL_GAMEPAD_AXIS_LEFTX: {
            mInputState.leftX = value;
            break;
        }
        case SDL_GAMEPAD_AXIS_LEFTY: {
            mInputState.leftY = value;
            break;
        }
        case SDL_GAMEPAD_AXIS_RIGHTX: {
            mInputState.rightX = value;
            break;
        }
        case SDL_GAMEPAD_AXIS_RIGHTY: {
            mInputState.rightY = value;
            break;
        }
        default: {
            break;
        }
    }
}
