#ifndef nbl_GEOMETRY_INC_GLSL
#define nbl_GEOMETRY_INC_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

// (Global) Vertex Data
// Declares a [buffer_reference] with type name "VertexBuffer"
// ============================================================

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec4 tangent;
};

// [VertexBuffer] Buffer Reference
layout (buffer_reference, scalar) readonly buffer VertexBuffer
{
    Vertex data[];
};

// (Global) Index Data
// Declares a [buffer_reference] with type name "IndexBuffer"
// ============================================================

// [IndexBuffer] Buffer Reference
layout (buffer_reference, scalar) readonly buffer IndexBuffer
{
    uint data[];
};

// Geometry Metadata
// Declares a [buffer_reference] with type name "GeometryBuffer"
// ============================================================

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

// [GeometryBuffer] Buffer Reference
layout (buffer_reference, scalar) readonly buffer GeometryBuffer
{
    GPUGeometryInfo data[];
};

#endif
