#version 460
#extension GL_EXT_mesh_shader                               : require
#extension GL_EXT_buffer_reference2                         : require
#extension GL_EXT_scalar_block_layout                       : enable
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

layout (
    local_size_x = nbl_Hair_Workgroup_Size
) in;
layout (
    triangles,
    max_vertices   = nbl_Hair_Mesh_MaxVertices,
    max_primitives = nbl_Hair_Mesh_MaxPrimitives
) out;

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
taskPayloadSharedEXT TaskPayload IN;

uint workGroupID = gl_WorkGroupID.x;
uint laneID      = gl_LocalInvocationID.x;

// Output -------------------------------
layout (location = 0) out MeshData m_out[];

// Functions ----------------------------
HairStrandDesc getStrandDescription(uint id)
{
    return strandDescBuffer.data[id];
}

uint getStrandletVertexCount(uint strandletID, uint totalVertexCount)
{
    return min(totalVertexCount - (nbl_Hair_Workgroup_Size * strandletID), nbl_Hair_Workgroup_Size);
}

void main()
{
    uint deltaID = 0;
    uint k       = 0;
    for (uint i = 0; i < nbl_Hair_Workgroup_Size; i++)
    {
        if (workGroupID < uint(IN.deltaID[i]))
        {
            break;
        }
        deltaID = uint(IN.deltaID[i]);
        k = i;
    }

    // Current [Strand] information
    uint            current_strandID    = IN.baseID + k;
    HairStrandDesc  strand_description  = getStrandDescription(current_strandID);
    uint            strand_vertex_count = strand_description.vertexCount;
    uint            base_vertex_offset  = strand_description.firstVertex;

    // Current [Strandlet] information
    uint strandletID        = workGroupID - deltaID;
    uint strandlet_vertices = getStrandletVertexCount(strandletID, strand_vertex_count);

    // Calculate output parameters
    uint n_quads = strandlet_vertices - 1;
    uint n_tri   = n_quads * 2;
    uint n_vtx   = n_quads * 4;

    // Do no work if current lane exceeds quad count
    if (laneID > n_quads)
    {
        return;
    }

    SetMeshOutputsEXT(n_vtx, n_tri);

    // Calculate global offset into the vertex buffer
    uint vertex_buffer_offset = base_vertex_offset + (strandletID * nbl_Hair_Workgroup_Size) + laneID - strandletID;

    vec4 strand_vertex = vec4(vertexBuffer.data[vertex_buffer_offset].position, 1.0);
    vec4 offset_vertex = strand_vertex + vec4(0.15, 0.0, 0.0, 0.0);
    vec4 next_vertex   = vec4(vertexBuffer.data[vertex_buffer_offset + 1].position, 1.0);

    vec4 tangent = vec4(strand_vertex.xyz - next_vertex.xyz, 0.0);

    // [Strand (0) | Offset (1) | Offset (2) | Strand (3)] ordered output
    // 0 ---- 1
    // |    / |
    // | /    |
    // 3 ---- 2
    vec4 A = (laneID % 2 == 0) ? strand_vertex : offset_vertex;
    vec4 B = (laneID % 2 == 0) ? offset_vertex : strand_vertex;

    const mat4 M  = model;
    const mat4 VP = cameraBuffer.data.proj * cameraBuffer.data.view;

    vec4 world_pos_A   = M * A;
    vec4 world_pos_B   = M * B;
    vec4 world_tangent = normalize(vec4((M * tangent).xyz, 0.0));

    const uint out_offset = laneID * 2;

    gl_MeshVerticesEXT[out_offset + 0].gl_Position = VP * world_pos_A;
    m_out[out_offset + 0].worldPosition = world_pos_A;
    m_out[out_offset + 0].worldTangent  = world_tangent;

    gl_MeshVerticesEXT[out_offset + 1].gl_Position = VP * world_pos_B;
    m_out[out_offset + 1].worldPosition = world_pos_B;
    m_out[out_offset + 1].worldTangent  = world_tangent;

    const uint tri_offset = laneID * 2;
    gl_PrimitiveTriangleIndicesEXT[tri_offset + 0] = uvec3(2, 1, 0) + out_offset;
    gl_PrimitiveTriangleIndicesEXT[tri_offset + 1] = uvec3(3, 2, 0) + out_offset;
}