#ifndef NBL_GLSL
#define NBL_GLSL

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct GPUInstanceData
{
    mat4        model;
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

// Mirroring the Vulkan struct
struct DrawIndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int  vertexOffset;
    uint firstInstance;
};

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