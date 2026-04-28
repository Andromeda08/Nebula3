#ifndef nbl_INSTANCE_INC_GLSL
#define nbl_INSTANCE_INC_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

// Instance Indirection Map
// Declares a [buffer_reference] with type name "InstanceIndirectionBuffer"
// ------------------------------------------------------------
// The instance indirection map isn't always required,
// to declare the buffer reference define "nbl_INSTANCE_INDIRECTION"
// before including this file.
// ------------------------------------------------------------
// Example usage:
// uint instanceBufferIndex = instanceIndirectionMap.data[gl_InstanceIndex];
// ============================================================

// [InstanceIndirectionBuffer] Buffer reference
#ifdef nbl_INSTANCE_INDIRECTION
layout (buffer_reference, scalar) readonly buffer InstanceIndirectionBuffer
{
    uint data[];
};
#endif

// Instance Data
// Declares a [buffer_reference] with type name "InstanceBuffer"
// ============================================================

struct GPUInstanceData
{
    mat4        model;
    mat4        modelInverse;
    mat4        modelPrevious;
    vec4        aabbMin;
    vec4        aabbMax;
    uint64_t    blas;
    int         geometryIndex;
    int         materialIndex;
    int         objectId;
};

// [InstanceBuffer] Buffer reference
layout (buffer_reference, scalar) readonly buffer InstanceBuffer
{
    GPUInstanceData data[];
};

#endif