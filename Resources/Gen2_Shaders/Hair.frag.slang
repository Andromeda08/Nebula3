#language slang 2026

// Workgroup Config
static const uint nbl_Hair_Workgroup_Size     = 32;
static const uint nbl_Hair_Max_StrandletSize  = nbl_Hair_Workgroup_Size;

// Mesh Shader Config
static const uint nbl_Hair_Mesh_MaxVertices   = 64;
static const uint nbl_Hair_Mesh_MaxPrimitives = 64;

enum RenderingMode
{
    Default             = 0,
    Debug_Quads         = 1,
    Debug_Strands       = 2,
    Debug_Strandlets    = 3,
};

struct MeshVertex
{
    float4 position : SV_Position;
    float3 worldPosition;
    float3 worldTangent;
};

struct CameraData
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewInverse;
    float4x4 projInverse;
    float4   eye;
    float4   frustumPlanes[6];
    float    nearPlane;
    float    farPlane;
};

struct HairVertex
{
    float3 position;
};

struct HairAttributes
{
    float3 color;
    float  thickness;
    float  transparency;
};

struct HairStrandDesc
{
    int  strandId;
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
    uint            _pad0;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

float3 kajiyaKay(float3 diffuse, float3 specular, float p, float3 T, float3 L, float3 V)
{
    float cosTL = dot(T, L);
    float sinTL = sqrt(1.0 - cosTL * cosTL);

    float3 H = normalize(L + V);
    float cosTH = dot(T, H);
    float sinTH = sqrt(1.0 - cosTH * cosTH);

    float3 d = diffuse * sinTL;
    float3 s = specular * pow(max(sinTH, 0.0), p);

    return d + s;
}

[shader("fragment")]
float4 main(MeshVertex input) : SV_Target
{
    const float3 lightPos = float3(-75.0, 125.0, 50.0);

    float3 T = normalize(input.worldTangent);
    float3 L = normalize(lightPos - input.worldPosition);
    float3 V = normalize(pc.cameraBuffer.eye.xyz - input.worldPosition);

    float3 color = kajiyaKay(pc.diffuse.rgb, pc.specular.rgb, 16.0, T, L, V);
    return float4(color, 1.0);
}
