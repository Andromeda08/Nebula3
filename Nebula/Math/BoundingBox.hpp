#pragma once

#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>


struct BoundingBox
{
    glm::vec4 min = glm::vec4(glm::vec3(std::numeric_limits<float>::infinity()), 1.0f);
    glm::vec4 max = glm::vec4(glm::vec3(-1.0f * std::numeric_limits<float>::infinity()), 1.0f);

    BoundingBox& expandBy(const glm::vec3 point)
    {

        return *this;
    }
};
