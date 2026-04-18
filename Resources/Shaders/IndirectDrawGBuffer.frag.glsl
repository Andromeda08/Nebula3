#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct GPUInstanceData
{
    mat4      model;
    vec4      boundsMin;
    vec4      boundsMax;
    uint64_t  blasAddress;
    int       materialIndex;
    int       geometryIndex;
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

layout (push_constant) uniform GBufferPushConstants {
    uint64_t instanceBufferAddress;
    uint64_t instanceMapAddress;
};

// Input Attributes
// ========================================
layout (location = 0) in      vec4 inViewPosition;
layout (location = 1) in      vec4 inViewNormal;
layout (location = 2) in      vec2 inUV;
layout (location = 3) in      vec3 inViewTangent;
layout (location = 4) in      vec3 inViewBitangent;
layout (location = 5) in flat uint inInstancePoolIndex;

// Output Attributes
// ========================================
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;
layout (location = 3) out vec4 outEmissive;
layout (location = 4) out vec4 outLightingParams;
layout (location = 5) out vec2 outMotionVector;

// Bound Resources
// ========================================
layout (set = 0, binding = 0) uniform CameraUniform {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    vec4  frustumPlanes[6];
    float nearPlane;
    float farPlane;
} camera;

layout (set = 0, binding = 2) readonly buffer MaterialData {
    GPUMaterialData materials[];
};

layout (buffer_reference, std430) readonly buffer InstanceBuffer {
    GPUInstanceData instances[];
};

#define MAX_TEXTURES          1024
#define TEXTURE_VALID         1
#define MISSING_TEXTURE_INDEX 0

layout (set = 1, binding = 0)       uniform          sampler2D uTextures[MAX_TEXTURES];
layout (set = 1, binding = 1, r32i) readonly uniform iimage2D  uTextureMeta;

float linearDepth(float depth)
{
    return (camera.nearPlane * camera.farPlane) /
        (camera.farPlane - depth * (camera.farPlane - camera.nearPlane));
}

void main()
{
    InstanceBuffer  instanceBuffer = InstanceBuffer(instanceBufferAddress);
    GPUInstanceData instanceData   = instanceBuffer.instances[inInstancePoolIndex];
    GPUMaterialData material       = materials[instanceData.materialIndex];

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

    vec3 N = normalize(inViewNormal.xyz);

    if (material.normalMapIndex >= 0)
    {
        int textureValidity = imageLoad(uTextureMeta, ivec2(material.normalMapIndex, 0)).r;
        if (textureValidity == TEXTURE_VALID)
        {
            mat3 TBN = mat3(normalize(inViewTangent), normalize(inViewBitangent), normalize(inViewNormal.xyz));
            vec3 normalMap = texture(uTextures[material.normalMapIndex], inUV).rgb;
            N = normalMap * 2.0 - 1.0;
            N = normalize(TBN * N);
        }
    }

    float metallic  = material.metallicFactor;
    float roughness = material.roughnessFactor;
    if (material.metallicRoughnessMapIndex >= 0)
    {
        int textureValidity = imageLoad(uTextureMeta, ivec2(material.metallicRoughnessMapIndex, 0)).r;
        if (textureValidity == TEXTURE_VALID)
        {
            vec4 textureColor = texture(uTextures[material.metallicRoughnessMapIndex], inUV);
            roughness *= textureColor.g;
            metallic  *= textureColor.b;
        }
    }

    outPosition       = vec4(inViewPosition.xyz, linearDepth(gl_FragCoord.z));
    outNormal         = vec4(N, 1.0);
    outAlbedo         = color;
    outEmissive       = material.isEmissive > 0 ? vec4(color.rgb, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
    outLightingParams = vec4(metallic, roughness, 0.0, 0.0);
}