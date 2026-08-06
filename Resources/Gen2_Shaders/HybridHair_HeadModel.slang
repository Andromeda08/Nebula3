#include "lib/Camera.slang"
#include "lib/Emitter.slang"

struct Vertex
{
    float3 position;
    float3 normal;
    float2 uv;
    float4 tangent;
};

struct VSOutput
{
    float4 position      : SV_Position;
    float3 worldPosition;
    float3 worldNormal;
};

struct HeadPushConstants
{
    Vertex*        vertexBuffer;
    GPUCameraData* cameraBuffer;
    GPULightData*  lights;
    float3         baseColor;
    uint           firstIndex;
    uint           firstVertex;
};

[[vk::push_constant]]
ConstantBuffer<HeadPushConstants> pc;

[shader("vertex")]
VSOutput vsMain(uint vertexId : SV_VertexID)
{
    Vertex v = pc.vertexBuffer[pc.firstVertex + vertexId];

    VSOutput output;
    float4 worldPos      = float4(v.position, 1.0);
    output.position      = mul(pc.cameraBuffer.proj, mul(pc.cameraBuffer.view, worldPos));
    output.worldPosition = worldPos.xyz;
    output.worldNormal   = normalize(v.normal);

    return output;
}

[shader("fragment")]
float4 fsMain(VSOutput input) : SV_Target
{
    float3 N = normalize(input.worldNormal);
    float3 L = -normalize(pc.lights[0].vector.xyz);

    float NdotL = max(dot(N, L), 0.0);

    float3 diffuse = pc.baseColor.rgb * NdotL * pc.lights[0].color * pc.lights[0].intensity;
    float3 ambient = pc.baseColor.rgb * 0.1;

    return float4(diffuse + ambient, 1.0);
}