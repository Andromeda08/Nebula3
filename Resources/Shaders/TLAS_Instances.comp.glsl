#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout (local_size_x = 64) in;

struct GPUInstanceData
{
    mat4     model;
    vec4     solidColor;
    int      textureIndex;
    int      geometryIndex;
    uint64_t blasAddress;
    int      normalIndex;
    int      _p0, _p1, _p2;
    vec4     min;
    vec4     max;
};

struct AccelerationStructureInstanceKHR
{
    float    transform[12];             // vk::TransformMatrixKHR
    uint     instanceCustomIndex_Mask;  // (24 bits, 8 bits)
    uint     sbtRecordOffset_Flags;     // (24 bits, 8 bits)
    uint64_t blasReference;
};

// Bound Resources
// ========================================
layout (push_constant) uniform TLASUpdatePushConstant { uint slots; };

layout (set = 0, binding = 0) readonly buffer Instances {
    GPUInstanceData instances[];
};
layout (set = 0, binding = 1) writeonly buffer TLASInstances {
    AccelerationStructureInstanceKHR tlasInstances[];
};

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= slots)
    {
        return;
    }

    GPUInstanceData instance = instances[idx];
    if (instance.blasAddress == 0)
    {
        tlasInstances[idx].instanceCustomIndex_Mask = 0;
        tlasInstances[idx].blasReference = 0;
        return;
    }

    mat4 model = instance.model;
    tlasInstances[idx].transform[0]  = model[0].x;
    tlasInstances[idx].transform[1]  = model[1].x;
    tlasInstances[idx].transform[2]  = model[2].x;
    tlasInstances[idx].transform[3]  = model[3].x;
    tlasInstances[idx].transform[4]  = model[0].y;
    tlasInstances[idx].transform[5]  = model[1].y;
    tlasInstances[idx].transform[6]  = model[2].y;
    tlasInstances[idx].transform[7]  = model[3].y;
    tlasInstances[idx].transform[8]  = model[0].z;
    tlasInstances[idx].transform[9]  = model[1].z;
    tlasInstances[idx].transform[10] = model[2].z;
    tlasInstances[idx].transform[11] = model[3].z;

    tlasInstances[idx].instanceCustomIndex_Mask = (0xFF << 24) | (idx & 0x00FFFFFF);
    tlasInstances[idx].sbtRecordOffset_Flags    = (0x01 << 24);

    tlasInstances[idx].blasReference = instance.blasAddress;
}