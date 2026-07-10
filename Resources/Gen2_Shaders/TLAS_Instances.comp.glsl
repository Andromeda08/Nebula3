#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

layout (local_size_x = 64) in;

#include "nbl/Instance.inc.glsl"
#include "nbl/Vulkan.inc.glsl"

layout (buffer_reference, scalar) writeonly buffer TLASInstancesBuffer {
    AccelerationStructureInstanceKHR data[];
};

layout (push_constant) uniform PushConstants {
    TLASInstancesBuffer tlasInstances;
    InstanceBuffer      instances;
    uint                slots;
};

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= slots)
    {
        return;
    }

    GPUInstanceData instance = instances.data[idx];
    if (instance.blas == 0)
    {
        tlasInstances.data[idx].instanceCustomIndex_Mask = 0;
        tlasInstances.data[idx].blasReference = 0;
        return;
    }

    mat4 model = instance.model;
    tlasInstances.data[idx].transform[0]  = model[0].x;
    tlasInstances.data[idx].transform[1]  = model[1].x;
    tlasInstances.data[idx].transform[2]  = model[2].x;
    tlasInstances.data[idx].transform[3]  = model[3].x;
    tlasInstances.data[idx].transform[4]  = model[0].y;
    tlasInstances.data[idx].transform[5]  = model[1].y;
    tlasInstances.data[idx].transform[6]  = model[2].y;
    tlasInstances.data[idx].transform[7]  = model[3].y;
    tlasInstances.data[idx].transform[8]  = model[0].z;
    tlasInstances.data[idx].transform[9]  = model[1].z;
    tlasInstances.data[idx].transform[10] = model[2].z;
    tlasInstances.data[idx].transform[11] = model[3].z;

    tlasInstances.data[idx].instanceCustomIndex_Mask = (0xFF << 24) | (idx & 0x00FFFFFF);
    tlasInstances.data[idx].sbtRecordOffset_Flags    = (0x00 << 24);

    tlasInstances.data[idx].blasReference = instance.blas;
}