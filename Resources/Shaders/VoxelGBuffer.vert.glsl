#version 460

// Vertex Attributes
// ========================================
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUv;

// Instance Attributes
// ========================================
layout (location = 3) in mat4 inModel;
layout (location = 7) in vec4 inColor;

// Output Attributes
// ========================================
layout (location = 0) out vec4 outWorldPosition;
layout (location = 1) out vec4 outWorldNormal;
layout (location = 2) out vec2 outUv;
layout (location = 3) out vec4 outColor;

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
    vec3 normal = mat3(inModel) * inNormal;

    outWorldPosition = inModel * vec4(inPosition, 1.0);
    outWorldNormal   = vec4(normal, 0.0);
    outUv            = inUv;
    outColor         = inColor;
    gl_Position     = camera.proj * camera.view * outWorldPosition;
}