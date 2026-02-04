#version 460

// Input Attributes
// ========================================
layout (location = 0) in vec4 inWorldPosition;
layout (location = 1) in vec4 inWorldNormal;
layout (location = 2) in vec2 inUv;
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

void main()
{
    outPosition = inWorldPosition;
    outNormal   = inWorldNormal;
    outAlbedo   = vec4(inColor.rgb, 1.0);
}