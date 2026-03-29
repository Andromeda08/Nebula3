#language slang 2026

struct CameraData
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewInverse;
    float4x4 projInverse;
    float4   eye;
    float    nearPlane;
    float farPlane;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CameraData> camera;

[[vk::binding(2, 0)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(0, 1)]]
RWTexture2D<float4> outputImage;

struct Attributes
{
    float2 bary;
};

struct Payload
{
    float3 hitValue;
};

[shader("raygeneration")]
void rgenMain()
{
    uint3 launchID   = DispatchRaysIndex();
    uint3 launchSize = DispatchRaysDimensions();

    const float2 pixelCenter = float2(launchID.xy) + float2(0.5, 0.5);
    const float2 inUV = pixelCenter / float2(launchSize.xy);
    const float2 d = inUV * 2.0 - 1.0;

    float4 target = mul(camera.projInverse, float4(d.x, d.y, 1.0, 1.0));


    RayDesc rayDesc;
    rayDesc.Origin = mul(camera.viewInverse, float4(0.0, 0.0, 0.0, 1.0)).xyz;
    rayDesc.Direction = mul(camera.viewInverse, float4(normalize(target.xyz), 0.0)).xyz;
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 10000.0;

    Payload payload;
    TraceRay(tlas, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);

    outputImage[int2(launchID.xy)] = float4(payload.hitValue, 1.0);
    
}

[shader("closesthit")]
void closesthitMain(inout Payload p, in Attributes attribs)
{
    const float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    p.hitValue = barycentricCoords;

    float3 vertexPos0 = HitTriangleVertexPosition(0);
    float3 vertexPos1 = HitTriangleVertexPosition(1);
    float3 vertexPos2 = HitTriangleVertexPosition(2);
    float3 currentPos = vertexPos0 * barycentricCoords.x + vertexPos1 * barycentricCoords.y + vertexPos2 * barycentricCoords.z;

    p.hitValue = currentPos;
}

[shader("miss")]
void missMain(inout Payload p)
{
    p.hitValue = float3(0.0, 0.0, 0.0);
}
