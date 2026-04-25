#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace nbl
{
    struct Transform
    {
        glm::vec3 _translate    = glm::vec3(0.0f);
        glm::vec3 _scale        = glm::vec3(1.0f);
        glm::vec3 _euler        = glm::vec3(0.0f);

        glm::mat4 _model        = glm::mat4(1.0f);
        bool      _customModel  = false;

        glm::mat4 getModel() const
        {
            if (_customModel)
            {
                return _model;
            }

            const glm::mat4 T = glm::translate(glm::mat4(1.0f), _translate);
            const glm::mat4 S = glm::scale(glm::mat4(1.0f), _scale);
            const glm::mat4 R = glm::yawPitchRoll(glm::radians(_euler.y), glm::radians(_euler.x), glm::radians(_euler.z));
            return T * R * S;
        }

        Transform& translate(const glm::vec3& t) noexcept
        {
            _translate += t;
            return *this;
        }

        Transform& setTranslate(const glm::vec3& t) noexcept
        {
            _translate = t;
            return *this;
        }

        Transform& scale(const glm::vec3& s) noexcept
        {
            _scale += s;
            return *this;
        }

        Transform& setScale(const glm::vec3& s) noexcept
        {
            _scale = s;
            return *this;
        }

        Transform& rotate(const glm::vec3& r) noexcept
        {
            _euler += r;
            wrapRotationAngles();
            return *this;
        }

        Transform& setRotation(const glm::vec3& r) noexcept
        {
            _euler = r;
            wrapRotationAngles();
            return *this;
        }

    private:
        void wrapRotationAngles() noexcept
        {
            _euler.x = wrapAngle(_euler.x);
            _euler.y = wrapAngle(_euler.y);
            _euler.z = wrapAngle(_euler.z);
        }

        [[nodiscard]] static float wrapAngle(float a) noexcept
        {
            a = glm::mod(a, 360.0f);
            return a < 0 ? a + 360.0f : a;
        }
    };
}
