#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct CameraData
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

struct GPUInstanceData
{
    mat4      model;
    vec4      solidColor;
    vec4      min;
    vec4      max;
    uint64_t  blasAddress;
    int       textureIndex;
    int       geometryIndex;
    int       normalIndex;
    int       isEmissive;
    int       metallicRoughnessIndex;
    float     metallicFactor;
    float     roughnessFactor;
    int       _p0;
    int       _p1;
    int       _p2;
};

// Texture Cosntants
// ============================================
// Maximum number of textures contained in the descriptor array.
#define MAX_TEXTURES 1024
// Value in the meta texture marking if a texture slot is currently valid or not.
#define TEXTURE_VALID 1
// Index to default to when an invalid texture index is used.
#define MISSING_TEXTURE_INDEX 0


// Light Types and Constants
// ============================================
// Maximum number of lights contained in the descriptor array.
#define MAX_LIGHTS 256

struct GPULightData
{
    vec3  vector;
    int   lightType;
    vec3  color;
    float intensity;
    float radius;
    int   enabled;
    int   castsShadow;
    int   _p0;
};

// Utility Functions
// ============================================

/**
 * Compute linear depth using the given camera parameters.
 */
float getLinearDepth(float depth, float nearPlane, float farPlane)
{
    return (nearPlane * farPlane) / (farPlane - depth * (farPlane - nearPlane));
}

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