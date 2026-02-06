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

void FlyingCamera::onUpdate() noexcept
{
    const glm::vec3 right = glm::normalize(glm::cross(mOrientation, mUp));

    if (mKeyState.forward)
    {
        mEye += mSpeed * mOrientation;
    }
    if (mKeyState.backward)
    {
        mEye -= mSpeed * mOrientation;
    }
    if (mKeyState.left)
    {
        mEye -= mSpeed * right;
    }
    if (mKeyState.right)
    {
        mEye += mSpeed * right;
    }
    if (mKeyState.up)
    {
        mEye += (mSpeed / 2.0f) * mUp;
    }
    if (mKeyState.down)
    {
        mEye -= (mSpeed / 2.0f) * mUp;
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
            mKeyState.forward = pressed;
            break;
        }
        case SDL_SCANCODE_S: {
            mKeyState.backward = pressed;
            break;
        }
        case SDL_SCANCODE_A: {
            mKeyState.left = pressed;
            break;
        }
        case SDL_SCANCODE_D: {
            mKeyState.right = pressed;
            break;
        }
        case SDL_SCANCODE_SPACE: {
            mKeyState.up = pressed;
            break;
        }
        case SDL_SCANCODE_LCTRL: {
            mKeyState.down = pressed;
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
