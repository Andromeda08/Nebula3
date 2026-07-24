#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <SDL3/SDL.h>
#include "Camera.hpp"
#include "GPUCameraData.hpp"

namespace nbl
{
    class IsometricCamera : public Camera
    {
    public:
        IsometricCamera() : Camera("IsometricCamera") {}

        void onEvent(const SDL_Event& event) noexcept override
        {

        }

        void onUpdate() override
        {
            const uint64_t now = SDL_GetTicks();
            const float  dt  = (mLastTick == 0) ? 0.0f : (now - mLastTick) / 1000.0f;
            mLastTick = now;

            const bool* keys = SDL_GetKeyboardState(nullptr);

            const glm::vec3 forward = glm::normalize(glm::vec3(-glm::cos(sAzimuth), 0.0f, -glm::sin(sAzimuth)));
            const glm::vec3 right   = glm::normalize(glm::vec3( glm::sin(sAzimuth), 0.0f, -glm::cos(sAzimuth)));

            glm::vec3 move(0.0f);
            if (keys[SDL_SCANCODE_W]) move += forward;
            if (keys[SDL_SCANCODE_S]) move -= forward;
            if (keys[SDL_SCANCODE_D]) move += right;
            if (keys[SDL_SCANCODE_A]) move -= right;

            if (glm::length(move) > 0.0f)
            {
                mTarget += glm::normalize(move) * mPanSpeed * dt;
            }
        }

        [[nodiscard]] GPUCameraData getGpuCameraData() const override
        {
            const auto offset = glm::vec3 {
                mDistance * glm::cos(mElevation) * glm::cos(sAzimuth),
                mDistance * glm::sin(mElevation),
                mDistance * glm::cos(mElevation) * glm::sin(sAzimuth),
            };

            const glm::vec3 eye = mTarget + offset;

            constexpr float aspect = 16.0f / 9.0f;
            const glm::mat4 view = glm::lookAt(eye, mTarget, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 proj = glm::orthoRH_ZO(
                -mDistance * aspect, mDistance * aspect,
                -mDistance, mDistance, 0.1f, 1024.0f);
            proj[1][1] *= -1.0f;

            return GPUCameraData {
                .view           = view,
                .proj           = proj,
                .viewInverse    = glm::inverse(view),
                .projInverse    = glm::inverse(proj),
                .eye            = glm::vec4(eye, 1.0f),
                .nearPlane      = 0.1f,
                .farPlane       = 1024.0f,
            };
        }

    private:
        static constexpr float sAzimuth = glm::radians(45.0f);

        const float mElevation  = glm::atan(1.0f / glm::sqrt(2.0f));

        glm::vec3   mTarget     = glm::vec3(0.0f);
        float       mDistance   = 50.0f;
        float       mPanSpeed   = 15.0f;
        uint64_t    mLastTick   = 0;
    };
}
