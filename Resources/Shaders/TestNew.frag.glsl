#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

#include "inc/nbl.inc.glsl"

layout (location = 0) in  flat uint inInstancePoolIndex;
layout (location = 1) in       vec2 inUV;

layout (location = 0) out      vec4 outColor;

layout (buffer_reference, std430) readonly buffer InstanceBuffer {
    GPUInstanceData instances[];
};

layout (buffer_reference, std430) readonly buffer InstanceMapBuffer {
    uint indices[];
};

layout (buffer_reference, scalar) readonly buffer CameraDataBuffer {
    mat4 view;
    mat4 proj;
    mat4 viewInverse;
    mat4 projInverse;
    vec4 eye;
    vec4 frustumPlanes[6];
    float nearPlane;
    float farPlane;
};

layout (buffer_reference, scalar) readonly buffer MaterialBuffer {
    GPUMaterialData materials[];
};

layout (push_constant) uniform GBufferPushConstants {
    InstanceBuffer      instanceBuffer;
    InstanceMapBuffer   instanceMap;
    CameraDataBuffer    camera;
    MaterialBuffer      materialBuffer;
};

#define MAX_TEXTURES          1024
#define TEXTURE_VALID         1
#define MISSING_TEXTURE_INDEX 0

layout (set = 0, binding = 0)       uniform          sampler2D uTextures[MAX_TEXTURES];
layout (set = 0, binding = 1, r32i) readonly uniform iimage2D  uTextureMeta;


void main()
{
    GPUInstanceData instanceData = instanceBuffer.instances[inInstancePoolIndex];
    GPUMaterialData material     = materialBuffer.materials[instanceData.materialIndex];

    vec4 color = material.solidColor;
    if (material.textureIndex >= 0)
    {
        int textureValidity = imageLoad(uTextureMeta, ivec2(material.textureIndex, 0)).r;
        vec4 textureColor = (textureValidity == TEXTURE_VALID)
        ? texture(uTextures[material.textureIndex], inUV)
        : texture(uTextures[MISSING_TEXTURE_INDEX], inUV);

        if (textureColor.a < 0.5)
        {
            discard;
        }

        color = vec4(textureColor.rgb, 1.0);
    }

    outColor = vec4(color.rgb, 1.0);
}