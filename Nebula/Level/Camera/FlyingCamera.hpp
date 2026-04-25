#pragma once

// FlyingCamera.hpp
// Basic flying camera implementation.
// Left click to lookaround, WASD for movement, CTRL / Space to fly down / up.
// Alternatively Gamepad L, R, L1 and R1 for the same controls.
// ============================================================

#include <array>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include "Camera.hpp"
#include "GPUCameraData.hpp"

namespace nbl
{
    class FlyingCamera final : public Camera
    {
    public:
        FlyingCamera(glm::ivec2 size, glm::vec3 eye, float h_fov = 75.0f, float near = 0.1f, float far = 512.0f);

        ~FlyingCamera() override;

        void onEvent(const SDL_Event& event) noexcept override;

        void onUpdate() noexcept override;

        [[nodiscard]] GPUCameraData getGpuCameraData() const override;
    private:
        [[nodiscard]] glm::mat4 getView() const;

        [[nodiscard]] glm::mat4 getProjection() const;

        void handleKeyEvent(const SDL_KeyboardEvent& keyEvent) noexcept;

        void handleMouseEvent(const SDL_MouseMotionEvent& motionEvent) noexcept;

        void handleGamepadButtonEvent(const SDL_GamepadButtonEvent& buttonEvent) noexcept;

        void handleGamepadAxisEvent(const SDL_GamepadAxisEvent& axisEvent) noexcept;

        glm::ivec2 mSize;
        glm::vec3  mUp          = {0, 1, 0};
        glm::vec3  mOrientation = {0, 0, -1};

        float mNear;
        float mFar;
        float mFov;

        float mSpeed {0.25f};
        float mSensitivity {50.0f};
        float mDeadzone = 0.1f;

        bool mMouseCaptured = false;

        struct KeyState {
            bool forward  = false;
            bool backward = false;
            bool left     = false;
            bool right    = false;
            bool up       = false;
            bool down     = false;

            float leftX   = 0.0f;
            float leftY   = 0.0f;
            float rightX  = 0.0f;
            float rightY  = 0.0f;
        } mInputState;
    };
}