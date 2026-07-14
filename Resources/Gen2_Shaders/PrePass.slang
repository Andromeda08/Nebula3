#language slang 2026

#include "../Path/lib/types.slang"

static const int nbl_MAX_TEXTURES          = 1024;
static const int nbl_TEXTURE_VALID         = 1;
static const int nbl_MISSING_TEXTURE_INDEX = 0;

struct PushConstants
{
    GPUInstanceData* instances;
    uint*            instanceIndirectionMap;
    GPUCameraData*   camera;
    GPUMaterialData* materials;
};

[[vk::push_constant]] PushConstants pc;

[[vk::binding(0, 0)]] Sampler2D        uTextures[nbl_MAX_TEXTURES];
[[vk::binding(1, 0)]] RWTexture2D<int> uTextureValidity;

bool isTextureValid(int textureIndex)
{
    return uTextureValidity[int2(textureIndex, 0)] == nbl_TEXTURE_VALID;
}

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float4 tangent  : TANGENT;
};

struct VSOutput
{
    float4               clipPos       : SV_Position;
    nointerpolation uint instanceIndex : INSTANCE_INDEX;
    float2               uv            : TEXCOORD0;
};

[shader("vertex")]
VSOutput vert(VSInput input, uint instanceID : SV_VulkanInstanceID)
{
    let instanceIndex = pc.instanceIndirectionMap[instanceID];
    let instance      = pc.instances[instanceIndex];

    let worldPos = mul(instance.model, float4(input.position, 1.0));

    VSOutput output;
    output.clipPos       = mul(pc.camera.proj, mul(pc.camera.view, worldPos));
    output.instanceIndex = instanceIndex;
    output.uv            = input.uv;
    return output;
}

[shader("fragment")]
int2 frag(VSOutput input) : SV_Target
{
    let instance = pc.instances[input.instanceIndex];
    let material = pc.materials[instance.materialIndex];

    if (material.textureIndex >= 0)
    {
        let index = isTextureValid(material.textureIndex)
            ? material.textureIndex
            : nbl_MISSING_TEXTURE_INDEX;

        if (uTextures[index].Sample(input.uv).a < 0.5)
        {
            discard;
        }
    }

    return int2(instance.objectId, int(input.instanceIndex));
}