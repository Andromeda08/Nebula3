#version 460

// Input Attributes
// ========================================
layout (location = 0) in vec3 inPosition;

// Output Attributes
// ========================================
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec2 outUV;

// Bound Resources
// ========================================
layout (set = 0, binding = 0) uniform CameraData {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  position;
    float nearPlane;
    float farPlane;
    float _p0, _p1, _p3;
} cameraData;

void main()
{
    outPosition = vec4(inPosition, 1.0);
    outUV = vec2((inPosition.x + 1) / 2, (inPosition.y + 1) / 2);
}
