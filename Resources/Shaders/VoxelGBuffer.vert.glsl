#version 460

// Vertex Attributes
// ========================================
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

// Instance Attributes
// ========================================
layout (location = 3) in mat4 inModel;
layout (location = 7) in vec4 inColor;

// Output Attributes
// ========================================
layout (location = 0) out vec4 outViewPosition;
layout (location = 1) out vec4 outViewNormal;
layout (location = 2) out vec2 outUV;
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
    mat4 MV = camera.view * inModel;

    vec4 viewPos = MV * vec4(inPosition, 1.0);
    outViewPosition = viewPos;

    mat3 normalMatrix = transpose(inverse(mat3(MV)));
    outViewNormal = vec4(normalMatrix * inNormal, 0.0);

//    outViewPosition = inModel * vec4(inPosition, 1.0);
//
//    mat3 mNormal = transpose(inverse(mat3(inModel)));
//    outViewNormal = vec4(mNormal * normalize(inNormal), 0.0);

    outUV       = vec2(inUV.x, 1.0 - inUV.y);
    outColor    = inColor;
    gl_Position = camera.proj * viewPos;
}
