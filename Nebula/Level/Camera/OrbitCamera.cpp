#include "OrbitCamera.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace nbl
{
    void OrbitCamera::onEvent(const SDL_Event& event) noexcept
    {
        switch (event.type)
        {
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    mDragging = true;
                }
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    mDragging = false;
                }
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                if (mDragging)
                {
                    orbit(event.motion.yrel * mOrbitSpeed, event.motion.xrel * mOrbitSpeed);
                }
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                zoom(-event.wheel.y * mZoomSpeed);
                break;
            }
            default: {
                break;
            }
        }
    }

    void OrbitCamera::onUpdate()
    {
        mEye = {
            mRadius * glm::sin(mTheta) * glm::cos(mPhi),
            mRadius * glm::cos(mTheta),
            mRadius * glm::sin(mTheta) * glm::sin(mPhi),
        };

        mView = glm::lookAt(mEye, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        mProj       = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 1024.0f);
        mProj[1][1] *= -1;

        updateFrustumPlanes();
    }

    GPUCameraData OrbitCamera::getGpuCameraData() const
    {
        auto data = GPUCameraData {
            .view           = mView,
            .proj           = mProj,
            .viewInverse    = glm::inverse(mView),
            .projInverse    = glm::inverse(mProj),
            .eye            = glm::vec4(mEye, 1.0f),
            .nearPlane      = 0.1f,
            .farPlane       = 1024.0f,
        };

        // std::ranges::copy_n(std::begin(mFrustumPlanes), 6, std::begin(data.frustumPlanes));
        return data;
    }

    void OrbitCamera::orbit(const float dTheta, const float dPhi) noexcept
    {
        constexpr float epsilon = 1e-4f;
        mTheta                  = glm::clamp(mTheta + dTheta, epsilon, glm::pi<float>() - epsilon);
        mPhi                    += dPhi;
    }

    void OrbitCamera::zoom(const float delta) noexcept
    {
        mRadius = glm::max(0.01f, mRadius + delta);
    }
}
