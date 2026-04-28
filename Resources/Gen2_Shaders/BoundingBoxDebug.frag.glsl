#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

#include "nbl/Camera.inc.glsl"
#include "nbl/Instance.inc.glsl"

layout (location = 0) out vec4 outColor;

layout (push_constant) uniform PushConstants {
    vec4                boxColor;
    InstanceBuffer      instances;
    CameraBuffer        camera;
    uint                instanceGpuIndex;
};

void main()
{
    outColor = boxColor;
}