#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

#include "inc/nbl.inc.glsl"

layout (location = 0) out vec4 outColor;

layout (buffer_reference, scalar) readonly buffer CameraDataBuffer {
    GPUCameraData camera;
};

layout (buffer_reference, std430) readonly buffer InstanceBuffer {
    GPUInstanceData instances[];
};

layout (push_constant) uniform PushConstants {
    vec4                boxColor;
    CameraDataBuffer    cameraBuffer;
    InstanceBuffer      instanceData;
    uint                instanceGpuIndex;
    uint                _pad0;
};

void main()
{
    outColor = boxColor;
}