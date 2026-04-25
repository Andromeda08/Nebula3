#pragma once

#include <array>
#include <glm/glm.hpp>
#include <SDL3/SDL_events.h>
#include "GPUCameraData.hpp"

namespace nbl
{
    class Camera
    {
    public:
        explicit Camera(const std::string& name);

        virtual ~Camera() = default;

        virtual void onEvent(const SDL_Event& event) {}

        virtual void onUpdate() {}

        virtual GPUCameraData getGpuCameraData() const = 0;

        const glm::vec3& getEye() const;

        [[nodiscard]] const std::array<glm::vec4, 6>& getFrustumPlanes() const;

        [[nodiscard]] const std::string& getName() const noexcept;

    protected:
        void updateFrustumPlanes();

        std::string              mName          = "Unknown Camera";
        glm::mat4                mView          = glm::mat4(1.0f);
        glm::mat4                mProj          = glm::mat4(1.0f);
        glm::vec3                mEye           = glm::vec3(0.0f);
        std::array<glm::vec4, 6> mFrustumPlanes = {};
    };
}