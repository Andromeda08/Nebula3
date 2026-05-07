#version 460

#extension GL_EXT_buffer_reference                       : require
#extension GL_EXT_buffer_reference2                      : require
#extension GL_EXT_scalar_block_layout                    : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive                   : enable

#define nbl_Hair_VertexRef
#define nbl_Hair_StrandRef
#define nbl_Hair_DebugColorRef
#include "nbl/Hair.inc.glsl"

#include "nbl/Camera.inc.glsl"


// layout(early_fragment_tests) in;

layout (location = 0) in MeshData IN;

layout (push_constant) uniform PushConstants
{
    mat4                    model;
    vec4                    diffuse;
    vec4                    specular;
    HairVertexBuffer        vertexBuffer;
    HairStrandDescBuffer    strandDescBuffer;
    DebugColorsBuffer       debugColorBuffer;
    CameraBuffer            cameraBuffer;
    uint                    firstVertex;
    uint                    vertexCount;
    uint                    firstStrand;
    uint                    strandCount;
    uint                    renderMode;
    uint                    _pad0;
};


layout (location = 0) out vec4 outColor;

vec3 shift_tangent(vec3 T, vec3 N, float s)
{
    return normalize(T + s * N);
}

float strand_specular(vec3 T, vec3 V, vec3 L, float exponent)
{
    vec3  H     = normalize(L + V);
    float dotTH = dot(T, H);
    float sinTH = sqrt(1.0 - dotTH * dotTH);
    float dir_attenuation = smoothstep(-1.0, 0.0, dotTH);
    return dir_attenuation * pow(sinTH, exponent);
}

float strand_diffuse(vec3 N, vec3 L)
{
    return clamp(mix(0.25, 1.0, max(dot(N, L), 0.0)), 0.0, 1.0);
}

vec3 kajiya_kay(vec3 diffuse, vec3 specular, float p, vec3 T, vec3 L, vec3 V) {
    float cosTL    = dot(T, L);
    float sinTL    = sqrt(1.0f - cosTL * cosTL);

    vec3 H      = normalize(L + V);
    float cosTH = dot(T, H);
    float sinTH = sqrt(1.0 - cosTH * cosTH);

    vec3 d = diffuse * sinTL;
    vec3 s = specular * pow(max(sinTH, 0.0), p);

    return d + s;
}

void main()
{
    vec4 light = vec4(-75, 125, 50, 0);

    vec3 T = normalize(IN.worldTangent.xyz);
    vec3 L = normalize(light - IN.worldPosition).xyz;
    vec3 V = normalize(cameraBuffer.data.eye.xyz - IN.worldPosition.xyz);

    vec3 color = kajiya_kay(diffuse.rgb, specular.rgb, 16.0, T, L, V);
    outColor = vec4(color, 1.0);
}