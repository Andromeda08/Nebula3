#pragma once

#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace Math
{
    static float wrapAngle(float a)
    {
        a = glm::mod(a, 360.0f);
        return a < 0 ? a + 360.0f : a;
    }

    static void wrapRotationAngles(glm::vec3& euler)
    {
        euler.x = wrapAngle(euler.x);
        euler.y = wrapAngle(euler.y);
        euler.z = wrapAngle(euler.z);
    }
}
