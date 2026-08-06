#include "nbl/Hair.inc.slang"

[[vk::push_constant]]
ConstantBuffer<MeshPushConstants> pc;

[shader("fragment")]
float4 main(MeshVertex input) : SV_Target
{
    // Use single light at index 0 for shading, no matter the enable state.
    float3 hairColor = MarschnerBSDF(pc.lights[0], pc.camera[0], input.worldPosition.xyz, input.worldTangent.xyz, pc.bsdf);

    return float4(hairColor, 1.0);
}
