struct VSOutput
{
    float4 Pos : SV_Position;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
};

[shader("vertex")]
VSOutput main(uint VertexIndex : SV_VertexID)
{
    VSOutput output;
    output.UV  = float2((VertexIndex << 1) & 2, VertexIndex & 2);
    output.Pos = float4(output.UV * 2.0 - 1.0, 0.0, 1.0);
    return output;
}
