#version 460

// Input Attributes
// ========================================
layout (location = 0) in vec4 inWorldPosition;

// Bound Resources
// ========================================
layout (push_constant) uniform PushConstant {
    vec4 color;
};
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

// Input Attributes
// ========================================
layout (location = 0) out vec4 outColor;

vec3 gammaCorrection(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

void main()
{
    outColor = vec4(color.rgb, 1.0);
}