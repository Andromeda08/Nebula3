#version 460

#extension GL_EXT_mesh_shader                               : require
#extension GL_EXT_buffer_reference2                         : require
#extension GL_KHR_shader_subgroup_ballot                    : require
#extension GL_EXT_scalar_block_layout                       : enable
#extension GL_KHR_shader_subgroup_arithmetic                : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64    : require
#extension GL_GOOGLE_include_directive                      : enable

#ifdef DEBUG
    #extension GL_EXT_debug_printf : enable
#endif

#define nbl_Hair_VertexRef
#define nbl_Hair_StrandRef
#define nbl_Hair_DebugColorRef
#include "nbl/Hair.inc.glsl"

#include "nbl/Camera.inc.glsl"

layout (local_size_x = nbl_Hair_Workgroup_Size) in;

layout (push_constant) uniform PushConstants
{
    mat4                    model;
    vec4                    diffuse;
    vec4                    specular;
    HairVertexBuffer        vertexBuffer;
    HairStrandDescBuffer    strandDescBuffer;
    DebugColorsBuffer       debugColorBuffer;
    CameraBuffer            cameraBuffer;
    uint                    firstVertex;
    uint                    vertexCount;
    uint                    firstStrand;
    uint                    strandCount;
    uint                    renderMode;
    uint                    _pad0;
};

// Input --------------------------------
uint baseID = gl_WorkGroupID.x * nbl_Hair_Workgroup_Size;
uint laneID = gl_LocalInvocationID.x;

// Output -------------------------------
taskPayloadSharedEXT TaskPayload OUT;

// Functions ----------------------------
HairStrandDesc getStrandDescription(uint id)
{
    return strandDescBuffer.data[id];
}

void main()
{
    uint l_strandID = laneID;               // Relative to Workgroup (Local) Strand ID
    uint g_strandID = baseID + l_strandID;  // Global Strand ID

    if (g_strandID >= strandCount)
    {
        return;
    }

    HairStrandDesc strandDescription     = getStrandDescription(g_strandID);
    uint           strandletCount        = strandDescription.strandletCount;
    uint           strandWorkgroupOffset = subgroupExclusiveAdd(strandletCount);

    if (laneID != 0)
    {
        OUT.deltaID[laneID] = uint8_t(strandWorkgroupOffset);
    }
    OUT.baseID = baseID;

    // Task WG local
    uint sumStrandletCount = subgroupBroadcast(strandWorkgroupOffset + strandletCount, 31);

    // Launch (Local Strandlet Count) number of Mesh Shader Workgroups
    EmitMeshTasksEXT(sumStrandletCount, 1, 1);
}