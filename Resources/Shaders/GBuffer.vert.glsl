#version 460

// Vertex Attributes
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUv;

// Instance Attributes
layout (location = 3) in mat4 inModel;
layout (location = 7) in vec4 inColor;
layout (location = 8) in int  inTextureIndex;

// Output Attributes
layout (location = 0) out vec4 frWorldPosition;
layout (location = 1) out vec4 frWorldNormal;
layout (location = 2) out vec2 frUv;
layout (location = 3) out vec4 frColor;
layout (location = 4) out int  frTextureIndex;

layout (set = 1, binding = 1) uniform CameraUniform {
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

    frWorldPosition = inModel * vec4(inPosition, 1.0);
    frWorldNormal   = vec4(normal, 0.0);
    frUv            = inUv;
    frColor         = inColor;
    frTextureIndex  = inTextureIndex;
    gl_Position     = camera.proj * camera.view * frWorldPosition;
}