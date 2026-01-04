#pragma once

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ICamera.hpp"
#include "UserInterface/IComponent.hpp"

struct OrbitCameraCreateInfo
{
    glm::vec3   target   = glm::vec3(0.0f);
    float       distance = 5.0f;
    float       pitch    = 0.0f;
    float       yaw      = 0.0f;
    float       aspect   = 16.0f / 9.0f;
};

class OrbitCamera : public ICamera
{
public:
    nbl_CTOR(OrbitCamera);

    ~OrbitCamera() override = default;

    [[nodiscard]] const glm::vec3& eye() const override
    {
        return mPosition;
    }

    [[nodiscard]] glm::mat4 view() const override
    {
        return glm::lookAt(mPosition, mTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    [[nodiscard]] glm::mat4 projection() const override
    {
        return glm::perspective(glm::radians(75.0f), mAspect, 0.1f, 10000.0f);
    }

    [[nodiscard]] CameraData getCameraData() const override
    {
        const auto v = view();
        const auto p = projection();
        return {
            .view        = v,
            .proj        = p,
            .viewInverse = glm::inverse(v),
            .projInverse = glm::inverse(p),
            .eye         = glm::vec4(mPosition, 1.0f),
            .nearPlane   = 0.1f,
            .farPlane    = 10000.0f
        };
    }

    void registerKeys(GLFWwindow* pWindow) override
    {
        if (glfwGetKey(pWindow, GLFW_KEY_R) == GLFW_PRESS)
        {
            mTarget = glm::vec3( 0.0f);
            mDistance = 5.0f;
            mPitch = 0.0f;
            mYaw = 0.0f;
            updatePosition();
        }
    }

    void registerMouse(GLFWwindow* pWindow) override
    {
        double xPos, yPos;
        glfwGetCursorPos(pWindow, &xPos, &yPos);

        if (mIsFirstMouse)
        {
            mLastMouse = { xPos, yPos };
            mIsFirstMouse = false;
        }

        const glm::vec2 deltaMouse = { xPos - mLastMouse.x, yPos - mLastMouse.y };
        mLastMouse = { xPos, yPos };

        const bool isLeftPressed = glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_LEFT)     == GLFW_PRESS;
        if (isLeftPressed)
        {
            rotate(deltaMouse.y * sSensitivityRotate, deltaMouse.x * sSensitivityRotate);
        }

        const bool isMiddlePressed = glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
        if (isMiddlePressed)
        {
            pan({-deltaMouse.x * sSensitivityPan, deltaMouse.y * sSensitivityPan});
        }

        if (!isLeftPressed && !isMiddlePressed)
        {
            mIsFirstMouse = true;
        }
    }

private:
    static constexpr float sSensitivityRotate = 0.005f;
    void rotate(const float dPitch, const float dYaw) noexcept
    {
        mPitch = glm::clamp(mPitch + dPitch, -glm::pi<float>() / 2.0f + 0.1f, glm::pi<float>() / 2.0f - 0.1f);
        mYaw += dYaw;
        updatePosition();
    }

    static constexpr float sSensitivityPan = 0.01f;
    void pan(const glm::vec2 delta) noexcept
    {
        const glm::vec3 forward = glm::normalize(mTarget - mPosition);
        const glm::vec3 right   = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        const glm::vec3 up      = glm::cross(right, forward);

        mTarget += right * delta.x + up * delta.y;
        updatePosition();
    }

    void updatePosition() noexcept
    {
        mPosition = mTarget + mDistance * glm::vec3(
            glm::cos(mPitch) * glm::cos(mYaw),
            glm::sin(mPitch),
            glm::cos(mPitch) * glm::sin(mYaw));
    }

    friend class OrbitCameraComponent;

    glm::vec3       mPosition;
    glm::vec3       mTarget;
    float           mDistance;
    float           mPitch;
    float           mYaw;
    float           mAspect;

    glm::dvec2      mLastMouse;
    bool            mIsFirstMouse = true;
};

inline OrbitCamera::OrbitCamera(const OrbitCameraCreateInfo& createInfo)
: mTarget(createInfo.target)
, mDistance(createInfo.distance)
, mPitch(createInfo.pitch)
, mYaw(createInfo.yaw)
, mAspect(createInfo.aspect)
{
}

class OrbitCameraComponent final : public IComponent
{
public:
    explicit OrbitCameraComponent(OrbitCamera* pCamera)
    : mCamera(pCamera)
    {
    }

    ~OrbitCameraComponent() override = default;

    void draw() override
    {
        ImGui::Begin("OrbitCamera Params");
        {
            ImGui::SliderFloat("Distance", &mCamera->mDistance, 0.1f, 10.0f);
        }
        ImGui::End();
    }

private:
    OrbitCamera* mCamera;
};
