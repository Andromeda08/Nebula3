#language slang 2026

#include "nbl/Hair.inc.slang"

[[vk::push_constant]]
ConstantBuffer<MeshPushConstants> pc;

/**
 * Which strand within the [Task] workgroup this [Mesh] workgroup is rendering.
 *
 * Depending on how many points a complete hair strand consists of, multiple
 * mesh shader workgroups may be dispatched to render them. A mesh shader workgroup
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
    return pc.strandDescriptions[index];
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
}

float3 makeWorldTangent(HairVertex start, HairVertex end)
{
    float3 T = end.position - start.position;
    return normalize(mul((float3x3) pc.model, T));
}

float3 getViewDirection(HairVertex current)
{
    float4 worldPos = mul(pc.model, float4(current.position, 1.0));
    return normalize(pc.camera.eye.xyz - worldPos.xyz);
}

void appendSmallTriangle(
    float4 clip0, float4 clip1, float4 clip2,
    float2 s0,    float2 s1,    float2 s2,
    float3 w0,    float3 w1,    float3 w2,
    float3 tangent,
    float3 color,
    uint   primitiveId
)
{
    uint slot;
    InterlockedAdd(pc.smallTriangleCountBuffer[0], 1, slot);

    if (slot >= pc.maxSmallTriangles)
    {
        return;
    }

    ScreenSpaceTriangle tri;
    tri.v0 = float3(s0, clip0.z / clip0.w);
    tri.v1 = float3(s1, clip1.z / clip1.w);
    tri.v2 = float3(s2, clip2.z / clip2.w);
    tri.w0 = w0;
    tri.w1 = w1;
    tri.w2 = w2;

    tri.tangent = tangent;
    tri.color   = color;

    tri.primitiveId = primitiveId;

    pc.trianglesBuffer[slot] = tri;
}

[shader("mesh")]
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
    float4 v2 = v1 + offsetDir * 0.15; // nextAttribs.thickness;
    float4 v3 = v0 + offsetDir * 0.15; // currAttribs.thickness;

    const float4x4 VP = mul(pc.camera.proj, pc.camera.view);
    
    // For both vertices and triangles
    const uint outOffset = threadIndex * 2;

    float4 clip0 = mul(VP, v0);
    float2 s0    = toScreen(clip0, pc.viewport);
    float4 clip1 = mul(VP, v1);
    float2 s1    = toScreen(clip1, pc.viewport);
    float4 clip2 = mul(VP, v2);
    float2 s2    = toScreen(clip2, pc.viewport);
    float4 clip3 = mul(VP, v3);
    float2 s3    = toScreen(clip3, pc.viewport);

    if (pc.isHybrid == 1)
    {
        bool tri0_isSmall = isSmallTriangle(s0, s2, s1, pc.smallTriangleThreshold);
        bool tri1_isSmall = isSmallTriangle(s2, s3, s1, pc.smallTriangleThreshold);

        uint basePrimitiveId = (globalStrandIndex << 12) | (strandletIndex << 6) | (threadIndex * 2);

        if (tri0_isSmall)
        {
            appendSmallTriangle(
                clip0,  clip2,  clip1,
                s0,     s2,     s1,
                v0.xyz, v2.xyz, v1.xyz,
                T, currAttribs.color.rgb, basePrimitiveId + 0
            );
        }

        if (tri1_isSmall)
        {
            appendSmallTriangle(
                clip2,  clip3,  clip1,
                s2,     s3,     s1,
                v2.xyz, v3.xyz, v1.xyz,
                T, currAttribs.color.rgb, basePrimitiveId + 1
            );
        }

        tris[outOffset + 0] = tri0_isSmall ? uint3(0) + outOffset : uint3(0, 2, 1) + outOffset;
        tris[outOffset + 1] = tri1_isSmall ? uint3(0) + outOffset : uint3(2, 3, 1) + outOffset;
    }

    verts[outOffset + 0] = { clip0, v0.xyz, T, currAttribs.color.rgb };
    verts[outOffset + 1] = { clip3, v3.xyz, T, currAttribs.color.rgb };
    
    if (pc.isHybrid == 0)
    {
        tris[outOffset + 0] = uint3(0, 2, 1) + outOffset;
        tris[outOffset + 1] = uint3(2, 3, 1) + outOffset;
    }
}
