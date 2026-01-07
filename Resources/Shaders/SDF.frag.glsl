#version 460

// Input Attributes
// ========================================
layout (location = 0) in vec2 inUV;

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
    //vec4 ro = cameraData.position;

    //vec3 rd = (cameraData.projInverse * vec4(inUV * 2 - 1, 0, 1)).xyz;
    outColor = vec4(inUV.rg, 0.0, 0.5);
}