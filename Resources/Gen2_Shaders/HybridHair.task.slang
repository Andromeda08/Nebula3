#language slang 2026

#include "nbl/Hair.inc.slang"

[[vk::push_constant]]
ConstantBuffer<MeshPushConstants> pc;

groupshared TaskPayload taskOut;

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

    uint strandletCount        = getStrandDescription(pc, globalStrandIndex).strandletCount;
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
