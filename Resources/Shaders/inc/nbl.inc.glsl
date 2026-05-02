#ifndef NBL_GLSL
#define NBL_GLSL

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

// Texture Constants
// ============================================
// Maximum number of textures contained in the descriptor array.
#define MAX_TEXTURES 1024
// Value in the meta texture marking if a texture slot is currently valid or not.
#define TEXTURE_VALID 1
// Index to default to when an invalid texture index is used.
#define MISSING_TEXTURE_INDEX 0

// Light Constants
// ============================================
// Maximum number of lights that can contribute to lighting
#define MAX_LIGHTS 64
#define LIGHT_POINT 0
#define LIGHT_DIRECTIONAL 1

struct GPULightData
{
    vec3  vector;
    int   lightType;
    vec3  color;
    float intensity;
    int   isEnabled;
    int   castsShadows;
    float radius;
    int   _pad0;
};

struct GPUInstanceData
{
    mat4        model;
    mat4        previousModel;
    vec4        aabbMin;
    vec4        aabbMax;
    uint64_t    blas;
    int         geometryIndex;
    int         materialIndex;
    int         objectId;
    int         _pad0;
    int         _pad1;
    int         _pad2;
};

struct GPUGeometryInfo
{
    int   geometryIndex;
    uint  triangleCount;

    // VertexBuffer Region
    uint  firstVertex;
    uint  vertexCount;

    // IndexBuffer Region
    uint  firstIndex;
    uint  indexCount;
};

struct GPUMaterialData
{
    vec4    solidColor;
    int     textureIndex;
    int     normalMapIndex;
    int     metallicRoughnessMapIndex;
    float   metallicFactor;
    float   roughnessFactor;
    int     isEmissive;
    uint    rtHitGroup;
    int     _pad0;
};

struct GPUCameraData
{
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    vec4  frustumPlanes[6];
    float nearPlane;
    float farPlane;
};

float linearDepth(GPUCameraData camera, float depth)
{
    return (camera.nearPlane * camera.farPlane) /
        (camera.farPlane - depth * (camera.farPlane - camera.nearPlane));
}

// Mirroring the Vulkan struct
struct DrawIndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int  vertexOffset;
    uint firstInstance;
};

// View -> World space conversion for positions.
vec3 viewToWorldPosition(vec3 viewPosition, mat4 viewInverse)
{
    return (viewInverse * vec4(viewPosition, 1.0)).xyz;
}

// View -> World space conversion for normals.
vec3 viewToWorldNormal(vec3 viewNormal, mat4 viewInverse)
{
    return mat3(viewInverse) * viewNormal;
}

bool isVisible(vec3 mn, vec3 mx, vec4 planes[6])
{
    for (int i = 0; i < 6; ++i)
    {
        vec4 p = planes[i];
        vec3 pv = vec3(
            p.x >= 0.0 ? mx.x : mn.x,
            p.y >= 0.0 ? mx.y : mn.y,
            p.z >= 0.0 ? mx.z : mn.z);

        if (dot(p.xyz, pv) + p.w < 0.0)
        {
            return false;
        }
    }
    return true;
}

#endif