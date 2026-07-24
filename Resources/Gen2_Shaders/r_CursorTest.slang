#language slang 2026

struct r_CursorTestPushConstants
{
    float4x4 proj;
    float2   center;
    float    a;
    float2   viewportSize;
    int      cursorTextureIndex;
    float3   color;
    int      isHit;
    float    fade;
};

[[vk::push_constant]] r_CursorTestPushConstants pc;

[[vk::binding(0, 0)]] Sampler2D uTextures[1024];

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

[shader("vertex")]
VSOutput vert(uint vertexId : SV_VertexID)
{
    var uv     = float2(vertexId & 1, (vertexId >> 1) & 1);
    var pixel  = pc.center + (uv - 0.5) * float2(pc.a);

    VSOutput o;
    o.pos = mul(pc.proj, float4(pixel, 0.0, 1.0));
    o.uv  = uv;
    return o;
}

[shader("fragment")]
float4 frag(VSOutput input) : SV_Target
{
    return pc.cursorTextureIndex >= 0
        ? uTextures[pc.cursorTextureIndex].Sample(input.uv)
        : pc.isHit == 1 ? float4(pc.color, pc.fade) : float4(pc.color, 1.0);
}
