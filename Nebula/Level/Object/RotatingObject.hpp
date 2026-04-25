#pragma once

#include "Object.hpp"
#include "Core/Random.hpp"

namespace nbl
{
    struct RotatingObject : public Object
    {
        ~RotatingObject() override = default;

        void onUpdate(const float dt) noexcept override
        {
            transform.rotate(glm::vec3(mRotationFactor) * dt);
            isInstanceDirty = true;
        }

    private:
        const float mRotationFactor = Random::get(2.5f, 15.0f) * (Random::get<int32_t>() % 2 == 0 ? 1.0f : -1.0f);
    };
}
