#language slang 2026

namespace config
{
    static const uint uTextureSet = 0;
}

#include "lib/nbl.slang"

[[vk::binding(0, config::uTextureSet)]] Sampler2D        uTextures[nbl::MAX_TEXTURES];
[[vk::binding(1, config::uTextureSet)]] RWTexture2D<int> uTextureValidity;

struct GBufferPushConstants
{
    GPUInstanceData* instances;
    uint*            instanceIndirectionMap;
    GPUCameraData*   camera;
    GPUCameraData*   previousCamera;
    GPUMaterialData* materials;
    uint2            renderRes;
};

[[vk::push_constant]] GBufferPushConstants pc;

struct VSOutput
{
    nointerpolation uint instanceIndex : INSTANCE_INDEX;
    nointerpolation uint materialIndex;
    float3               wPosition;
    float3               wNormal;
    float3               wPositionPrev;
    float4               wTangent;
    float2               uv;
    float4               currClipPos : SV_Position;
    float4               prevClipPos;
};

[shader("vertex")]
VSOutput vert_GBuffer(Vertex input, uint instanceID : SV_VulkanInstanceID)
{
    VSOutput o;
    o.instanceIndex = getTranslatedInstanceIndex(instanceID, pc.instanceIndirectionMap);

    var instanceData  = pc.instances[o.instanceIndex];
    var normalMatrix  = transpose(float3x3(instanceData.modelInverse));

    var modelHandedness = sign(determinant(float3x3(instanceData.model)));
    var bitangentSign = input.tangent.w * modelHandedness;

    float4 position = float4(input.position, 1.0);

    o.materialIndex = instanceData.materialIndex;
    o.wPosition     = mul(instanceData.model,         position).xyz;
    o.wPositionPrev = mul(instanceData.modelPrevious, position).xyz;
    o.wNormal       = mul(normalMatrix, input.normal);
    o.wTangent      = float4(mul(normalMatrix, input.tangent.xyz), bitangentSign);
    o.uv            = input.uv;
    o.currClipPos   = mul(pc.camera.getViewProj(),         float4(o.wPosition, 1.0));
    o.prevClipPos   = mul(pc.previousCamera.getViewProj(), float4(o.wPosition, 1.0));

    return o;
}

struct FSOutput
{
    float4 wPosition        : SV_Target0;
    float4 wNormal          : SV_Target1;
    float4 motionVectors    : SV_Target2;
    float  viewZ            : SV_Target3;
    float4 albedo           : SV_Target4;
    uint   emissiveMask     : SV_Target5;
    float2 lightingParams   : SV_Target6;
};

[shader("fragment")]
FSOutput frag_GBuffer(VSOutput input)
{
    FSOutput o;
    o.wPosition = float4(input.wPosition, 1.0);

    var material = pc.materials[input.materialIndex];
    var s        = material.sample(uTextures, uTextureValidity, input.wNormal, input.wTangent, input.uv);

    if (s.albedo.a < 0.5)
    {
        discard;
    }

    float viewZ     = mul(pc.camera.view        , o.wPosition                     ).z;
    float viewZPrev = mul(pc.previousCamera.view, float4(input.wPositionPrev, 1.0)).z;

    o.wNormal        = float4(s.normal, 0.0);
    o.motionVectors  = nbl::computeMotionVector(input.currClipPos, input.prevClipPos, viewZPrev - viewZ, pc.renderRes);
    o.viewZ          = viewZ;
    o.albedo         = float4(s.albedo.rgb, 1.0);
    o.emissiveMask   = select(material.isEmissive == 1, 1, 0);
    o.lightingParams = float2(s.metallic, s.roughness);

    return o;
}
