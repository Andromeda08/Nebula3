#language slang 2026

// Workgroup Config
static const uint nbl_Hair_Workgroup_Size     = 32;
static const uint nbl_Hair_Max_StrandletSize  = nbl_Hair_Workgroup_Size;

// Mesh Shader Config
static const uint nbl_Hair_Mesh_MaxVertices   = 64;
static const uint nbl_Hair_Mesh_MaxPrimitives = 64;

enum RenderingMode
{
    Default             = 0,
    Debug_Quads         = 1,
    Debug_Strands       = 2,
    Debug_Strandlets    = 3,
};

struct TaskPayload
{
    // Global strand index of the first strand in the [Task] workgroup.
    uint    baseID;
    
    // Exclusive prefix sum of strandlet counts within this [Task] workgroup.
    uint8_t deltaID[nbl_Hair_Workgroup_Size];
};

struct MeshVertex
{
    float4 position : SV_Position;
    float3 worldPosition;
    float3 worldTangent;
};

struct CameraData
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewInverse;
    float4x4 projInverse;
    float4   eye;
    float4   frustumPlanes[6];
    float    nearPlane;
    float    farPlane;
};

struct HairVertex
{
    float3 position;
};

struct HairAttributes
{
    float3 color;
    float  thickness;
    float  transparency;
};

struct HairStrandDesc
{
    int  strandId;
    uint vertexCount;
    uint strandletCount;
    uint firstVertex;
};

struct PushConstants
{
    float4x4        model;
    float4          diffuse;
    float4          specular;
    HairVertex*     vertexBuffer;
    HairAttributes* attribBuffer;
    HairStrandDesc* strandDescBuffer;
    float4*         debugColorBuffer;
    CameraData*     cameraBuffer;
    uint            hairFirstVertex;
    uint            hairVertexCount;
    uint            hairFirstStrand;
    uint            hairStrandCount;
    RenderingMode   renderingMode;
    uint            _pad0;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

/**
 * Which strand within the [Task] wourkgroup this [Mesh] workgroup is rendering.
 *
 * Depending on how many points a complete hair strand consists of, multiple
 * mesh shader workgroups may be dispatced to render them. A mesh shader workgroup
 * is responsible for rendering a single strandlet.
 *
 * E.g. for strands 0 (2 strandlets), 1 (3 strandlets) and 2 (4 strandlets)
 * deltaID = [0, 2, 5, 9, ...]
 *  strand 0 -> mesh wg 0
 *  strand 1 -> mesh wg 2
 */
uint getLocalStrandIndex(TaskPayload inPayload, uint workgroupID)
{
    uint localStrandIndex = 0;
    for (uint i = 0; i < nbl_Hair_Workgroup_Size; i++)
    {
        if (workgroupID < uint(inPayload.deltaID[i]))
        {
            break;
        }
        localStrandIndex = i;
    }
    return localStrandIndex;
}

/**
 * Convert from local to global strand index that can be used to index the
 * strand description buffer.
 *>
 * baseID is the strand first processed by the [Task] workgroup,
 * therefore the global index is just (baseID + local offset)
 */
uint getGlobalStrandIndex(TaskPayload inPayload, uint localStrandIndex)
{
    return inPayload.baseID + localStrandIndex;
}

/**
 * Which strandlet this [Mesh] workgroup should be rendering?
 */
uint getStrandletIndex(TaskPayload inPayload, uint workgroupID, uint localStrandIndex)
{
    return workgroupID - uint(inPayload.deltaID[localStrandIndex]);
}

/**
 * Fetch the strand description for the specified index
 * from the global storage buffer.
 */
HairStrandDesc getStrandDescription(uint globalStrandIndex)
{
    // A single buffer is used for all hair models, we need the global index for that buffer.
    uint index = pc.hairFirstStrand + globalStrandIndex;
    return pc.strandDescBuffer[index];
}

/**
 * Strandlet vertex count is either the Workgroup size (e.g. 32) the remainder
 * after subtracting (strandletIndex * workgroup size) from the total vertex count.
 */
uint getStrandletVertexCount(uint strandletIndex, HairStrandDesc strandDesc)
{
    const uint stride = nbl_Hair_Workgroup_Size - 1;
    return min(
        strandDesc.vertexCount - (stride * strandletIndex),
        nbl_Hair_Workgroup_Size
    );
}

uint getGlobalHairBufferIndex(uint strandletIndex, uint threadIndex, HairStrandDesc strandDesc)
{
    uint globalBaseOffset = pc.hairFirstVertex;
    uint hairRelativeOffset = strandDesc.firstVertex;

    // Strandlets share their seam vertex
    const uint stride = nbl_Hair_Workgroup_Size - 1;

    return globalBaseOffset + hairRelativeOffset + (strandletIndex * stride) + threadIndex;
    // return globalBaseOffset
    //     + hairRelativeOffset
    //     + (strandletIndex * nbl_Hair_Workgroup_Size) + threadIndex
    //     - strandletIndex;
}

float3 makeWorldTangent(HairVertex start, HairVertex end)
{
    float3 T = end.position - start.position;
    return normalize(mul((float3x3) pc.model, T));
}

float3 getViewDirection(HairVertex current)
{
    float4 worldPos = mul(pc.model, float4(current.position, 1.0));
    return normalize(pc.cameraBuffer.eye.xyz - worldPos.xyz);
}

[numthreads(nbl_Hair_Workgroup_Size, 1, 1)]
[outputtopology("triangle")]
void main(
    // Workgroup
    uint  threadIndex   : SV_GroupIndex,
    uint3 groupID       : SV_GroupID,
    // Mesh Shader
    in  payload  TaskPayload inPayload,
    out vertices MeshVertex  verts[nbl_Hair_Mesh_MaxVertices],
    out indices  uint3       tris[nbl_Hair_Mesh_MaxPrimitives]
)
{
    // Resolve strand & strandlet indices
    const uint workgroupID       = groupID.x;
    const uint localStrandIndex  = getLocalStrandIndex(inPayload, workgroupID);
    const uint globalStrandIndex = getGlobalStrandIndex(inPayload, localStrandIndex);
    const uint strandletIndex    = getStrandletIndex(inPayload, workgroupID, localStrandIndex);

    // Resolve strand & strandlet data
    HairStrandDesc strandDescription    = getStrandDescription(globalStrandIndex);
    uint           strandletVertexCount = getStrandletVertexCount(strandletIndex, strandDescription);

    // Output params
    uint outQuads = strandletVertexCount - 1;   // Processing 32 points, but emitting 31 quads.
    uint outTris  = outQuads * 2;
    uint outVerts = outQuads * 4;

    // Each thread produces one quad, so if the current threadIndex is
    // greater than the quad count we early return.
    if (threadIndex > outQuads)
    {
        return;
    }

    SetMeshOutputCounts(outVerts, outTris);

    // Read vertices and attributes
    uint           bufferIndex = getGlobalHairBufferIndex(strandletIndex, threadIndex, strandDescription);
    
    HairVertex     currVertex  = pc.vertexBuffer[bufferIndex];
    HairAttributes currAttribs = pc.attribBuffer[bufferIndex];

    HairVertex     nextVertex  = pc.vertexBuffer[bufferIndex + 1];
    HairAttributes nextAttribs = pc.attribBuffer[bufferIndex + 1];

    // Quad construction
    //
    // 0 ----- 3
    // |    /  |
    // | /     |
    // 1 ----- 2
    //
    // 0 is created from the current position
    // 1 is created from the next position
    // 3 and 2 are constructed from 0 and 1 offset by their respecitve
    // thickness in the direction cross(View, Tangent)

    float3 T_out = makeWorldTangent(currVertex, nextVertex);
    float3 T_in;

    const bool isStrandRoot = (strandletIndex == 0) && (threadIndex == 0);
    if (isStrandRoot)
    {
        T_in = T_out;
    }
    else
    {
        HairVertex prevVertex = pc.vertexBuffer[bufferIndex - 1];
        T_in = makeWorldTangent(prevVertex, currVertex);
    }

    float3 T = normalize(T_in + T_out);
    float3 V = getViewDirection(currVertex);

    float4 offsetDir = float4(normalize(cross(V, T)), 0.0);

    float4 v0 = mul(pc.model, float4(currVertex.position, 1.0));
    float4 v1 = mul(pc.model, float4(nextVertex.position, 1.0));

    // Technically we don't need v2 here :)
    float4 v2 = v1 + offsetDir * nextAttribs.thickness;
    float4 v3 = v0 + offsetDir * currAttribs.thickness;

    const float4x4 VP = mul(pc.cameraBuffer.proj, pc.cameraBuffer.view);
    
    // For both vertices and triangles
    const uint outOffset = threadIndex * 2;

    verts[outOffset + 0] = { mul(VP, v0), v0.xyz, T };
    verts[outOffset + 1] = { mul(VP, v3), v3.xyz, T };
    
    tris[outOffset + 0] = uint3(0, 2, 1) + outOffset;
    tris[outOffset + 1] = uint3(2, 3, 1) + outOffset;
}
