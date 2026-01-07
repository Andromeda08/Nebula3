#version 460

// Input Attributes
// ========================================
layout (location = 0) in vec2 frUV;

// Output Attributes
// ========================================
layout (location = 0) out vec4 outColor;

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

layout (set = 0, binding = 1) uniform sampler3D SDFTexture;

void main()
{
    //outColor = vec4(vec2(frUV), 0.f, 1.f);
    outColor = vec4(1.f, 0.f, 0.f, 1.f);
}