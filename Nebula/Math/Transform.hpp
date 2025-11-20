#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "MathUtil.hpp"

struct Transform
{
    glm::vec3 _translate = glm::vec3(0.0f);
    glm::vec3 _scale     = glm::vec3(1.0f);
    glm::vec3 _euler     = glm::vec3(0.0f);

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
        _dirty = true;
        return *this;
    }

    Transform& setRotation(const glm::vec3& r) noexcept
    {
        _euler = r;
        _dirty = true;
        return *this;
    }

    const glm::mat4& getModel() noexcept
    {
        if (_dirty)
        {
            _model = computeModel();
        }
        return _model;
    }

private:
    glm::mat4 computeModel() const
    {
        const glm::mat4 T = glm::translate(glm::mat4(1.0f), _translate);
        const glm::mat4 S = glm::scale(glm::mat4(1.0f), _scale);
        const glm::mat4 R = glm::yawPitchRoll(glm::radians(_euler.y), glm::radians(_euler.x), glm::radians(_euler.z));
        return T * R * S;
    }

    // _model is updated when _dirty is true and getModel() is called.
    glm::mat4 _model = glm::mat4(1.0f);
    bool      _dirty = false;
};
