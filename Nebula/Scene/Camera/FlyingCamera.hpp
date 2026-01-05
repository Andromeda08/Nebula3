#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "ICamera.hpp"

class FlyingCamera final : public ICamera
{
public:
    FlyingCamera(glm::ivec2 size, glm::vec3 eye, float h_fov = 75.0f, float near = 0.1f, float far = 10000.0f)
    : ICamera(), mSize(size), mEye(eye), mNear(near), mFar(far), mFov(h_fov) {}

    ~FlyingCamera() override = default;

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

    void registerKeys(GLFWwindow* pWindow) override
    {
        // WASD movement
        if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS)
        {
            mEye += mSpeed * mOrientation;
        }
        if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS)
        {
            mEye += mSpeed * -glm::normalize(glm::cross(mOrientation, mUp));
        }
        if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS)
        {
            mEye += mSpeed * -mOrientation;
        }
        if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS)
        {
            mEye += mSpeed * glm::normalize(glm::cross(mOrientation, mUp));
        }

        // Move up & down
        if (glfwGetKey(pWindow, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            mEye += (mSpeed / 2.0f) * mUp;
        }
        if (glfwGetKey(pWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        {
            mEye -= (mSpeed / 2.0f) * mUp;
        }
    }

    void registerMouse(GLFWwindow* pWindow) override
    {
        if (glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

            if (mClick)
            {
                // glfwSetCursorPos(pWindow, (mSize.x / 2), (mSize.y / 2));
                glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                mClick = false;
            }

            double mouseX, mouseY;
            glfwGetCursorPos(pWindow, &mouseX, &mouseY);

            float rotX = mSensitivity * (float)(mouseY - (mSize.y / 2)) / mSize.y;
            float rotY = mSensitivity * (float)(mouseX - (mSize.x / 2)) / mSize.x;

            glm::vec3 newOrientation = glm::rotate(mOrientation, glm::radians(-rotX), glm::normalize(glm::cross(mOrientation, mUp)));

            if (abs(glm::angle(newOrientation, mUp) - glm::radians(90.0f)) <= glm::radians(85.0f))
            {
                mOrientation = newOrientation;
            }

            mOrientation = glm::rotate(mOrientation, glm::radians(-rotY), mUp);
            glfwSetCursorPos(pWindow, (mSize.x / 2), (mSize.y / 2));
        }
        else if (glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
        {
            glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mClick = true;
        }
    }

private:
    glm::ivec2 mSize;
    glm::vec3  mEye;
    glm::vec3  mUp = {0, 1, 0};
    glm::vec3  mOrientation = {0, 0, -1};

    float mNear;
    float mFar;
    float mFov;

    float mSpeed {0.25f};
    float mSensitivity {50.0f};

    bool mClick {false};
};