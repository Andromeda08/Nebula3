#ifndef nbl_LIGHT_INC_GLSL
#define nbl_LIGHT_INC_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

// Light Constants
// ======================================
// Maximum number of lights that can contribute to lighting
#define MAX_LIGHTS        64
#define LIGHT_POINT        0
#define LIGHT_DIRECTIONAL  1

// Light Data
// Declares a [buffer_reference] with type name "LightBuffer"
// ============================================================

struct GPULightData
{
    vec3  vector;
    int   lightType;
    vec3  color;
    float intensity;
    int   isEnabled;
    int   castsShadows;
    float radius;
};

// [LightBuffer] Buffer Reference
layout (buffer_reference, scalar) readonly buffer LightBuffer
{
    GPULightData data[];
};

#endif
