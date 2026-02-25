#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "MathUtil.hpp"

namespace vk
{
    struct TransformMatrixKHR;
}

struct Transform
{
    glm::vec3 _translate = glm::vec3(0.0f);
    glm::vec3 _scale     = glm::vec3(1.0f);
    glm::vec3 _euler     = glm::vec3(0.0f);
    glm::vec3 _axis      = glm::vec3(0.0f);
    float     _angle     = 0.f;
    glm::quat _quat      = glm::quat(1, 0, 0, 0);

    Transform& translate(const glm::vec3& t) noexcept
    {
        _translate += t;
        _dirty = true;
        return *this;
    }

    Transform& setTranslate(const glm::vec3& t) noexcept
    {
        _translate = t;
        _dirty = true;
        return *this;
    }

    Transform& scale(const glm::vec3& s) noexcept
    {
        _scale += s;
        _dirty = true;
        return *this;
    }

    Transform& setScale(const glm::vec3& s) noexcept
    {
        _scale = s;
        _dirty = true;
        return *this;
    }

    Transform& rotate(const glm::vec3& r) noexcept
    {
        _euler += r;
        wrapRotationAngles();
        _dirty = true;
        return *this;
    }

    Transform& setRotation(const glm::vec3& r) noexcept
    {
        _euler = r;
        wrapRotationAngles();
        _dirty = true;
        return *this;
    }

    Transform& setAxisAngleRotation(const glm::vec3& ax, float an) noexcept
    {
        _axis = ax;
        _angle = an;
        _dirty = true;
        _useAxisAngle = true;
        return *this;
    }

    Transform& setRotation(const glm::quat& q) noexcept
    {
        _quat = q;
        _useQuat = true;
        _dirty = true;
        return *this;
    }

    Transform& setModel(const glm::mat4& model) noexcept
    {
        _model = model;
        _customModel = true;
        _dirty = true;
        return *this;
    }

    const glm::mat4& getModel() noexcept
    {
        if (_dirty && !_customModel)
        {
            _model = computeModel();
        }
        _dirty = false;
        return _model;
    }

    vk::TransformMatrixKHR getModel3x4() noexcept;

    [[nodiscard]] bool isDirty() const noexcept
    {
        return _dirty;
    }

private:
    glm::mat4 computeModel() const
    {
        const glm::mat4 T = glm::translate(glm::mat4(1.0f), _translate);
        const glm::mat4 S = glm::scale(glm::mat4(1.0f), _scale);
        const glm::mat4 R = _useAxisAngle 
            ? glm::rotate(glm::mat4(1.0f), glm::radians(_angle), _axis) 
            : _useQuat
                ? glm::mat4_cast(_quat)
                : glm::yawPitchRoll(glm::radians(_euler.y), glm::radians(_euler.x), glm::radians(_euler.z));
        return T * R * S;
    }

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

    // _model is updated when _dirty is true and getModel() is called.
    glm::mat4 _model = glm::mat4(1.0f);
    bool      _customModel = false;
    bool      _dirty = false;
    bool      _useAxisAngle = false;
    bool      _useQuat = false;
};
