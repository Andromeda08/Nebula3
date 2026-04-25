#include "Camera.hpp"

#include <glm/gtc/matrix_access.hpp>

namespace nbl
{
    Camera::Camera(const std::string& name)
    : mName(name)
    {
    }

    const glm::vec3& Camera::getEye() const
    {
        return mEye;
    }

    const std::array<glm::vec4, 6>& Camera::getFrustumPlanes() const
    {
        return mFrustumPlanes;
    }

    const std::string& Camera::getName() const noexcept
    {
        return mName;
    }

    void Camera::updateFrustumPlanes()
    {
        const auto viewProj = mProj * mView;
        const glm::vec4 row0 = glm::row(viewProj, 0);
        const glm::vec4 row1 = glm::row(viewProj, 1);
        const glm::vec4 row2 = glm::row(viewProj, 2);
        const glm::vec4 row3 = glm::row(viewProj, 3);

        std::array frustumPlanes = {
            row3 + row0, // left
            row3 - row0, // right
            row3 + row1, // bottom
            row3 - row1, // top
            row3 + row2, // near
            row3 - row2  // far
        };

        for (auto& p : frustumPlanes)
        {
            if (const auto length = glm::length(glm::vec3(p)); length > 0.0f)
            {
                p /= length;
            }
        }

        mFrustumPlanes = frustumPlanes;
    }
}
