#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

#define nbl_INSTANCE_INDIRECTION
#include "nbl/Instance.inc.glsl"

#define nbl_CAMERA_HISTORY
#include "nbl/Camera.inc.glsl"

#define nbl_TEXTURE_SET 0
#include "nbl/Texture.inc.glsl"

#include "nbl/Material.inc.glsl"

// Input Attributes
// ======================================
layout (location = 0) in flat uint inInstanceIndex;
layout (location = 1) in      vec3 inWorldPosition;
layout (location = 2) in      vec3 inWorldPositionPrev;
layout (location = 3) in      vec3 inWorldNormal;
layout (location = 4) in      vec4 inWorldTangent;
layout (location = 5) in      vec2 inUV;
layout (location = 6) in      vec4 inCurrClip;
layout (location = 7) in      vec4 inPrevClip;

// Output Attributes
// ======================================
layout (location = 0) out vec4  outWorldPosition;    // RGBA32  Flaot
layout (location = 1) out vec4  outWorldNormal;      // RGBA32  Float
layout (location = 2) out vec4  outMotionVectors;    // RGBA32  Float [XY pixel, Z lin. depth delta, W unused]
layout (location = 3) out float outViewZ;            // R32    Float
layout (location = 4) out vec4  outAlbedo;           // RGBA16 Float [XYZ Color, W alpha clip]
layout (location = 5) out uint  outEmissiveMask;     // R8     UInt8
layout (location = 6) out vec2  outLightingParams;   // RG16   Float [X Metallic, Y Roughness]

layout (push_constant) uniform PushConstants
{
    InstanceBuffer            instances;
    InstanceIndirectionBuffer instanceIndirectionMap;
    CameraBuffer              camera;
    HistoryCameraBuffer       previousCamera;
    MaterialBuffer            materials;
    uvec2                     renderRes;
};

struct MaterialSample
{
    vec4  albedo;
    vec3  normal;
    float metallic;
    float roughness;
};

MaterialSample sampleMaterial(GPUMaterialData material, mat3 TBN, vec2 uv)
{
    MaterialSample s;

    // Albedo
    s.albedo = vec4(material.solidColor.rgb, 1.0);
    if (material.textureIndex >= 0)
    {
        s.albedo = isTextureValid(material.textureIndex)
            ? texture(uTextures[material.textureIndex],     uv)
            : texture(uTextures[nbl_MISSING_TEXTURE_INDEX], uv);
    }
    
    // Normal Mapping
    s.normal = TBN[2];
    if (material.normalMapIndex >= 0)
    {
        if (isTextureValid(material.normalMapIndex))
        {
            vec3 textureRead = texture(uTextures[material.normalMapIndex], uv).rgb;
            s.normal = normalize(TBN * (textureRead * 2.0 - 1.0));
        }
    }

    // Metallic & Roughness Map
    s.metallic  = material.metallicFactor;
    s.roughness = material.roughnessFactor;
    if (material.metallicRoughnessMapIndex >= 0)
    {
        if (isTextureValid(material.metallicRoughnessMapIndex))
        {
            vec4 textureRead = texture(uTextures[material.metallicRoughnessMapIndex], uv);
            s.metallic  *= textureRead.b;
            s.roughness *= textureRead.g;
        }
    }

    return s;
}

vec4 computeMotionVector(vec4 currClip, vec4 prevClip, float deltaViewZ)
{
    vec2 currNDC = currClip.xy / currClip.w;
    vec2 currUV  = currNDC * 0.5 + 0.5;

    vec2 prevNDC = prevClip.xy / prevClip.w;
    vec2 prevUV  = prevNDC * 0.5 + 0.5;

    return vec4(
        (prevUV - currUV) * vec2(renderRes),
        deltaViewZ,
        0
    );
}

mat3 reorthogonalizeTBN(vec3 normal, vec4 tangent_bsign)
{
    vec3 N = normalize(normal);
    vec3 T = normalize(tangent_bsign.xyz - dot(tangent_bsign.xyz, N) * N);
    vec3 B = cross(N, T) * tangent_bsign.w;
    return mat3(T, B, N);
}

void main()
{
    GPUInstanceData instanceData = instances.data[inInstanceIndex];
    GPUMaterialData materialData = materials.data[instanceData.materialIndex];

    mat3 TBN = reorthogonalizeTBN(inWorldNormal, inWorldTangent);

    MaterialSample s = sampleMaterial(materialData, TBN, inUV);
    
    // Alpha clip
    if (s.albedo.a < 0.5)
    {
        discard;
    }

    // ViewZ
    float viewZ     = (camera.data.view * vec4(inWorldPosition, 1.0)).z;
    float viewZPrev = (previousCamera.data.view * vec4(inWorldPositionPrev, 1.0)).z;

    vec4 color = vec4(s.albedo.rgb, 1.0);

    outWorldPosition    = vec4(inWorldPosition, 1.0);
    outWorldNormal      = vec4(s.normal, 0.0);
    outMotionVectors    = computeMotionVector(inCurrClip, inPrevClip, viewZPrev - viewZ);
    outViewZ            = viewZ;
    outAlbedo           = color;
    outEmissiveMask     = (materialData.isEmissive == 1) ? 1 : 0;
    outLightingParams   = vec2(s.metallic, s.roughness);
}
