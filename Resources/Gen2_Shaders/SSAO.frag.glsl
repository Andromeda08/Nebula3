#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

#define nbl_SSAO_SET         0
#define nbl_SSuAO_KERNEL_SIZE 32
#define nbl_SSAO_RADIUS      1.0

#include "nbl/Camera.inc.glsl"

// Input Attributes
// ========================================
layout (location = 0) in vec2 inUV;

// Output Attributes
// ========================================
layout (location = 0) out float outColor;

// Bound Resources
// ========================================
layout (buffer_reference, scalar) readonly buffer SSAOKernel {
    vec4 samples[nbl_SSAO_KERNEL_SIZE];
};

layout (push_constant) uniform PushConstants {
    CameraBuffer camera;
    SSAOKernel   SSAO_Kernel;
};

layout (set = nbl_SSAO_SET, binding = 0) uniform sampler2D uSSAO_Noise;
layout (set = nbl_SSAO_SET, binding = 1) uniform sampler2D uWorldPosition;
layout (set = nbl_SSAO_SET, binding = 2) uniform sampler2D uWorldNormal;
layout (set = nbl_SSAO_SET, binding = 3) uniform sampler2D uViewZ;

void main()
{
    float viewZ = texture(uViewZ, inUV).r;
    if (viewZ >= 0.0)
    {
        outColor = 1.0;
        return;
    }

    vec3 worldPosition   = texture(uWorldPosition,  inUV).xyz;
    vec3 worldNormal     = texture(uWorldNormal,    inUV).xyz;

    vec3 viewPos    = worldToView_Point(worldPosition, camera.data);
    vec3 viewNormal = worldToView_Vector(worldNormal, camera.data);

    ivec2 texDim    = textureSize(uWorldPosition, 0);
    ivec2 noiseDim  = textureSize(uSSAO_Noise, 0);
    vec2 noiseScale = vec2(float(texDim.x) / float(noiseDim.x), float(texDim.y) / float(noiseDim.y));
    vec3 randomVec  = texture(uSSAO_Noise, inUV * noiseScale).xyz * 2.0 - 1.0;

    vec3 tangent = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    vec3 bitangent = cross(viewNormal, tangent);
    mat3 TBN = mat3(tangent, bitangent, viewNormal);

    float occlusion = 0.0;
    const float bias = 0.025;

    for (int i = 0; i < nbl_SSAO_KERNEL_SIZE; i++)
    {
        vec3 samplePos = viewPos + TBN * SSAO_Kernel.samples[i].xyz * nbl_SSAO_RADIUS;

        vec4 offset = camera.data.proj * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        vec2  sampleUV    = clamp(offset.xy, 0.0, 1.0);
        float sampleDepth = texture(uViewZ, sampleUV).r;

        float rangeCheck = smoothstep(0.0, 1.0, nbl_SSAO_RADIUS / abs(viewZ - sampleDepth));
        occlusion += (sampleDepth > samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    outColor = 1.0 - (occlusion / float(nbl_SSAO_KERNEL_SIZE));
}