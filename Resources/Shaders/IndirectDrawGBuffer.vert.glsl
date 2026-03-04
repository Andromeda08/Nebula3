#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct GPUInstanceData
{
    mat4     model;
    vec4     solidColor;
    int      textureIndex;
    int      geometryIndex;
    uint64_t blasAddress;
    int      normalIndex;
    int      _p0, _p1, _p2;
};

// Vertex Attributes
// ========================================
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inTangent;

// Output Attributes
// ========================================
layout (location = 0) out vec4 outViewPosition;
layout (location = 1) out vec4 outViewNormal;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec4 outColor;
layout (location = 4) out int  outTextureIndex;
layout (location = 5) out int  outNormalIndex;
layout (location = 6) out vec3 outViewTangent;
layout (location = 7) out vec3 outViewBitangent;

// Buffer References
// ========================================
layout (buffer_reference, std430) readonly buffer InstanceBuffer {
    GPUInstanceData instances[];
};

layout (buffer_reference, std430) readonly buffer InstanceMapBuffer {
    uint indices[];
};

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

layout (push_constant) uniform GBufferPushConstants {
    uint64_t instanceBufferAddress;
    uint64_t instanceMapAddress;
};

void main()
{
    InstanceMapBuffer map = InstanceMapBuffer(instanceMapAddress);
    InstanceBuffer buf = InstanceBuffer(instanceBufferAddress);

    uint poolIndex = map.indices[gl_InstanceIndex];
    GPUInstanceData inst = buf.instances[poolIndex];

    mat4 MV = camera.view * inst.model;

    vec4 viewPos = MV * vec4(inPosition, 1.0);
    outViewPosition = viewPos;

    mat3 normalMatrix = transpose(inverse(mat3(MV)));
    outViewNormal = vec4(normalMatrix * inNormal, 0.0);

    outUV            = vec2(inUV.x, 1.0 - inUV.y);
    //outUV            = vec2(inUV.x, inUV.y);
    outColor         = inst.solidColor;
    outTextureIndex  = inst.textureIndex;
    outNormalIndex   = inst.normalIndex;
    outViewTangent   = normalize(normalMatrix * inTangent.xyz);
    outViewBitangent = normalize(normalMatrix * cross(inNormal, inTangent.xyz) * inTangent.w);
    gl_Position      = camera.proj * viewPos;
}