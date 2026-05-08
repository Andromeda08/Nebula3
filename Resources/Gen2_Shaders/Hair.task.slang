#language slang 2026

// Workgroup Config
static const uint nbl_Hair_Workgroup_Size     = 32;
static const uint nbl_Hair_Max_StrandletSize  = nbl_Hair_Workgroup_Size;

enum RenderingMode
{
    Default = 0,
    Debug_Quads = 1,
    Debug_Strands = 2,
    Debug_Strandlets = 3,
};

struct TaskPayload
{
    // Global strand index of the first strand in the [Task] workgroup.
    uint baseID;

    // Exclusive prefix sum of strandlet counts within this [Task] workgroup.
    uint8_t deltaID[nbl_Hair_Workgroup_Size];
};

struct CameraData
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewInverse;
    float4x4 projInverse;
    float4 eye;
    float4 frustumPlanes[6];
    float nearPlane;
    float farPlane;
};

struct HairVertex
{
    float3 position;
};

struct HairAttributes
{
    float3 color;
    float thickness;
    float transparency;
};

struct HairStrandDesc
{
    int strandId;
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
    float           specularFactor;
    int             _pad0;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

groupshared TaskPayload taskOut;

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

[shader("amplification")]
[numthreads(nbl_Hair_Workgroup_Size, 1, 1)]
[require(spvGroupNonUniformArithmetic, spvGroupNonUniformShuffle)]
void main(
    uint  threadIndex : SV_GroupIndex,
    uint3 groupID     : SV_GroupID
)
{
    const uint baseID = groupID.x * nbl_Hair_Workgroup_Size;

    const uint localStrandIndex  = threadIndex;
    const uint globalStrandIndex = baseID + localStrandIndex;
    
    if (globalStrandIndex >= pc.hairStrandCount)
    {
        return;
    }

    uint strandletCount        = getStrandDescription(globalStrandIndex).strandletCount;
    uint strandWorkgroupOffset = WavePrefixSum(strandletCount);

    if (threadIndex != 0)
    {
        taskOut.deltaID[threadIndex] = uint8_t(strandWorkgroupOffset);
    }
    taskOut.baseID = baseID;

    GroupMemoryBarrierWithGroupSync();

    uint sumStrandletCount = WaveReadLaneAt(strandWorkgroupOffset + strandletCount, nbl_Hair_Workgroup_Size - 1);
    DispatchMesh(sumStrandletCount, 1, 1, taskOut);
}
