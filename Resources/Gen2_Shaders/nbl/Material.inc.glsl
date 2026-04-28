#ifndef nbl_MATERIAL_INC_GLSL
#define nbl_MATERIAL_INC_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

// Material Data
// Declares a [buffer_reference] with type name "MaterialBuffer"
// ============================================================

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
};

// [MaterialBuffer] Buffer Reference
layout (buffer_reference, scalar) readonly buffer MaterialBuffer
{
    GPUMaterialData data[];
};

#endif
