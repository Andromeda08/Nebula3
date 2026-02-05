#version 460

// Input Attributes
// ========================================
layout (location = 0) in vec4 inViewPosition;
layout (location = 1) in vec4 inViewNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inColor;

// Input Attributes
// ========================================
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

// Bound Resources
// ========================================
layout (set = 0, binding = 0) uniform CameraUniform {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    float nearPlane;
    float farPlane;
} camera;

float linearDepth(float depth)
{
    return (camera.nearPlane * camera.farPlane) /
        (camera.farPlane - depth * (camera.farPlane - camera.nearPlane));
}

void main()
{
    outPosition = vec4(inViewPosition.xyz, linearDepth(gl_FragCoord.z));
    outNormal   = vec4(normalize(inViewNormal.xyz), 1.0);
    outAlbedo   = inColor;
}