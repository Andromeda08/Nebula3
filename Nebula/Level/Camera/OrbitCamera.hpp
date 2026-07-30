#pragma once

// OrbitCamera.hpp
// Basic orbit camera implementation. Left click to drag,
// scroll to zoom.
// ============================================================

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <SDL3/SDL.h>
#include "Camera.hpp"
#include "GPUCameraData.hpp"

namespace nbl
{
    class OrbitCamera : public Camera
    {
    public:
        explicit OrbitCamera(const float initDistance = 5.0f)
        : Camera("OrbitCamera")
        , mRadius(initDistance)
        {
        }

        void onEvent(const SDL_Event& event) noexcept override;

        void onUpdate() override;

        [[nodiscard]] GPUCameraData getGpuCameraData() const override;

    private:
        void orbit(float dTheta, float dPhi) noexcept;

        void zoom(float delta) noexcept;

        float                    mPhi           = 0.0f;
        float                    mTheta         = glm::half_pi<float>();
        float                    mRadius        = 5.0f;
        bool                     mDragging      = false;
        float                    mOrbitSpeed    = 0.005f;
        float                    mZoomSpeed     = 1.5f;
    };
}
