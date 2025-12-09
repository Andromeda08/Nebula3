#pragma once

#include <glm/glm.hpp>

struct CameraData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
    glm::vec4 eye;
    float     nearPlane;
    float     farPlane;
};