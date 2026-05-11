#language slang 2026

struct PushConstants
{
    uint*  counter;
    uint3* indirectArgs;
    uint   groupSize;
    uint   maxGroups;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    uint count = pc.counter[0];
    uint gx    = min((count + pc.groupSize - 1) / pc.groupSize, pc.maxGroups);
    pc.indirectArgs[0] = uint3(gx, 1, 1);
}
