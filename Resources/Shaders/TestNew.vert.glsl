#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

#include "inc/nbl.inc.glsl"

// Vertex Attributes
// ========================================
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inTangent;

// Output Attributes
// ========================================
layout (location = 0) out uint outInstancePoolIndex;
layout (location = 1) out vec2 outUV;

// Buffer References
// ========================================
layout (buffer_reference, std430) readonly buffer InstanceBuffer {
    GPUInstanceData instances[];
};

layout (buffer_reference, std430) readonly buffer InstanceMapBuffer {
    uint indices[];
};

layout (buffer_reference, scalar) readonly buffer CameraDataBuffer {
    mat4 view;
    mat4 proj;
    mat4 viewInverse;
    mat4 projInverse;
    vec4 eye;
    vec4 frustumPlanes[6];
    float nearPlane;
    float farPlane;
};

layout (buffer_reference, scalar) readonly buffer MaterialBuffer {
    GPUMaterialData materials[];
};

layout (push_constant) uniform GBufferPushConstants {
    InstanceBuffer      instanceBuffer;
    InstanceMapBuffer   instanceMap;
    CameraDataBuffer    camera;
    MaterialBuffer      materialBuffer;
};


void main()
{
    uint poolIndex = instanceMap.indices[gl_InstanceIndex];
    GPUInstanceData inst = instanceBuffer.instances[poolIndex];

    mat4 MV = camera.view * inst.model;

    vec4 viewPos = MV * vec4(inPosition, 1.0);

    outInstancePoolIndex = poolIndex;
    outUV                = inUV;
    gl_Position          = camera.proj * viewPos;
}