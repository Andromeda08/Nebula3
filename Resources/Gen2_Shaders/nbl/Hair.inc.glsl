#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require

// Workgroup Config
// ======================================
#define nbl_Hair_Workgroup_Size         32
#define nbl_Hair_Max_Strandlet_size     nbl_Hair_Workgroup_Size

// Rendering Mode Enum
// ======================================
#define nbl_Hair_Mode_Default           0
#define nbl_Hair_Mode_DebugQuads        1
#define nbl_Hair_Mode_DebugStrands      2
#define nbl_Hair_Mode_DebugStrandlets   3

// Mesh Shader Output Config
// ======================================
#define nbl_Hair_Mesh_MaxVertices       64
#define nbl_Hair_Mesh_MaxPrimitives     64

// Debug Colors
// ======================================
#define nbl_Hair_Debug_Color_Count      1024

// Task Shader Payload
// ======================================
struct TaskPayload
{
    uint    baseID;
    uint8_t deltaID[nbl_Hair_Workgroup_Size - 1];
};

// Mesh Shader Payload
// ======================================
struct MeshData
{
    vec4 worldPosition;
    vec4 worldTangent;
};

struct MeshDataDebug
{
    vec4 worldPosition;
    vec4 worldTangent;
    vec4 color;
};

// GPU-side Hair Data
// ======================================
struct HairVertex
{
    vec3 position;
};

#ifdef nbl_Hair_VertexRef
layout (buffer_reference, scalar) readonly buffer HairVertexBuffer
{
    HairVertex data[];
};
#endif

struct HairAttributes
{
    vec3  color;
    float thickness;
    float transparency;
};

#ifdef nbl_Hair_AttributesRef
layout (buffer_reference, scalar) readonly buffer HairAttributesBuffer
{
    HairAttributes data[];
};
#endif

struct HairStrandDesc
{
    int  strandId;
    uint vertexCount;
    uint strandletCount;
    uint firstVertex;
};

#ifdef nbl_Hair_StrandRef
layout (buffer_reference, scalar) readonly buffer HairStrandDescBuffer
{
    HairStrandDesc data[];
};
#endif

// Global Hair Data Indirection
// ======================================
struct GlobalHairInfo
{
    uint firstVertex;
    uint vertexCount;

    uint firstAttribute;
    uint attributeCount;

    uint firstStrand;
    uint strandCount;
};

// Debug Color
// ======================================

#ifdef nbl_Hair_DebugColorRef
layout (buffer_reference, scalar) readonly buffer DebugColorsBuffer
{
    vec4 data[];
};
#endif