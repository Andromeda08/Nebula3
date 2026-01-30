#version 460

// Input Attributes
// ========================================
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in mat4 inModel;
layout (location = 7) in vec4 inColor;
layout (location = 8) in int  inTextureIndex;

// Output Attributes
// ========================================
layout (location = 0) out vec4 outWorldPosition;
layout (location = 1) out vec4 outColor;

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
    outWorldPosition = vec4(inPosition, 1.0);
    outColor         = inColor;
    gl_Position      = cameraData.proj * cameraData.view * inModel * outWorldPosition;
}
