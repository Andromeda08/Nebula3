#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

// ============================================
#define nbl_RT
// #define nbl_AMBIENT_OCCLUSION
// ============================================

#define nbl_TEMP_SKY_COLOR vec3(0.01)
#define nbl_ROUGHNESS_MIN 0.089

#ifdef nbl_RT
    #extension GL_EXT_ray_tracing : require
    #extension GL_EXT_ray_query : require
#endif

#define nbl_INPUT_SET   0
#define nbl_TEXTURE_SET 1
#define nbl_TLAS_SET    2

#define nbl_INSTANCE_INDIRECTION
#include "nbl/Instance.inc.glsl"

#include "nbl/Camera.inc.glsl"
#include "nbl/Constants.inc.glsl"
#include "nbl/CookTorrance.inc.glsl"
#include "nbl/Geometry.inc.glsl"
#include "nbl/Light.inc.glsl"
#include "nbl/Material.inc.glsl"
#include "nbl/Random.inc.glsl"
#include "nbl/Texture.inc.glsl"

#ifdef nbl_RT
    #include "nbl/TLAS.inc.glsl"
#endif

// Input Attributes
// ======================================
layout (location = 0) in  vec2 inUV;

// Output Attributes
// ======================================
layout (location = 0) out vec4 outColor;

layout (push_constant) uniform PushConstants {
    InstanceBuffer              instances;
    InstanceIndirectionBuffer   instanceIndirectionMap;
    CameraBuffer                camera;
    MaterialBuffer              materials;
    LightBuffer                 lights;
    VertexBuffer                vertexBuffer;
    IndexBuffer                 indexBuffer;
    GeometryBuffer              geometryBuffer;
    int                         shadowsEnabled;
    int                         sampleCount;
    int                         enableGI;
    float                       ambientFactor;
    float                       shadowFactor;
    float                       emissiveFactor;
};

layout (set = nbl_INPUT_SET, binding = 0) uniform sampler2D uWorldPosition;
layout (set = nbl_INPUT_SET, binding = 1) uniform sampler2D uWorldNormal;
layout (set = nbl_INPUT_SET, binding = 2) uniform sampler2D uAlbedo;
layout (set = nbl_INPUT_SET, binding = 3) uniform sampler2D uLightingParams;
layout (set = nbl_INPUT_SET, binding = 4) uniform sampler2D uViewZ;

#ifdef nbl_AMBIENT_OCCLUSION
layout (set = nbl_INPUT_SET, binding = 5) uniform sampler2D uAO;
#endif

vec3 offsetRay(vec3 p, vec3 n)
{
    //return origin + dir * 0.01;
    const float intScale   = 256.0;
    const float floatScale = 1.0 / 65536.0;
    const float origin     = 1.0 / 32.0;

    ivec3 of_i = ivec3(intScale * n.x, intScale * n.y, intScale * n.z);

    vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0.0) ? -of_i.x : of_i.x)),
                    intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0.0) ? -of_i.y : of_i.y)),
                    intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0.0) ? -of_i.z : of_i.z)));

    return vec3(abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,
                abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,
                abs(p.z) < origin ? p.z + floatScale * n.z : p_i.z);
}

mat3 reorthogonalizeTBN(vec3 normal, vec4 tangent_bsign)
{
    vec3 N = normalize(normal);
    vec3 T = normalize(tangent_bsign.xyz - dot(tangent_bsign.xyz, N) * N);
    vec3 B = cross(N, T) * tangent_bsign.w;
    return mat3(T, B, N);
}

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

struct HitSurface
{
    vec3  position;
    vec3  normal;
    vec3  albedo;
    vec3  emissive;
    float metallic;
    float roughness;
};

HitSurface reconstructHitSurface(rayQueryEXT rq)
{
    int    instanceId   = rayQueryGetIntersectionInstanceCustomIndexEXT(rq, true);
    int    primitiveId  = rayQueryGetIntersectionPrimitiveIndexEXT(rq, true);
    vec2   barycentrics = rayQueryGetIntersectionBarycentricsEXT(rq, true);
    mat4x3 objToWorld   = rayQueryGetIntersectionObjectToWorldEXT(rq, true);

    GPUInstanceData instance = instances.data[instanceId];
    GPUMaterialData material = materials.data[instance.materialIndex];
    GPUGeometryInfo geometry = geometryBuffer.data[instance.geometryIndex];

    uint   base = geometry.firstIndex + uint(primitiveId) * 3u;
    uint   i0   = indexBuffer.data[base + 0u] + geometry.firstVertex;
    uint   i1   = indexBuffer.data[base + 1u] + geometry.firstVertex;
    uint   i2   = indexBuffer.data[base + 2u] + geometry.firstVertex;

    Vertex v0   = vertexBuffer.data[i0];
    Vertex v1   = vertexBuffer.data[i1];
    Vertex v2   = vertexBuffer.data[i2];

    vec3 b  = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    vec3 p  = b.x * v0.position    + b.y * v1.position    + b.z * v2.position;
    vec3 n  = b.x * v0.normal      + b.y * v1.normal      + b.z * v2.normal;
    vec3 t  = b.x * v0.tangent.xyz + b.y * v1.tangent.xyz + b.z * v2.tangent.xyz;
    vec2 uv = b.x * v0.uv          + b.y * v1.uv          + b.z * v2.uv;
    
    vec3  worldNormal   = normalize(mat3(objToWorld) * n);
    vec3  worldTangent  = normalize(mat3(objToWorld) * t);
    float bitangentSign = v0.tangent.w;

    mat3 TBN = reorthogonalizeTBN(worldNormal, vec4(worldTangent, bitangentSign));

    MaterialSample s = sampleMaterial(material, TBN, uv);

    HitSurface hit;
    hit.position  = objToWorld * vec4(p, 1.0);
    hit.normal    = TBN[2];
    hit.albedo    = vec3(0.0);
    hit.emissive  = vec3(0.0);
    hit.metallic  = 0.0;
    hit.roughness = 0.0;
    
    if (s.albedo.a >= 0.5)
    {
        hit.albedo    = s.albedo.rgb;
        hit.emissive  = (material.isEmissive == 1) ? (s.albedo.rgb * emissiveFactor) : vec3(0.0);
        hit.metallic  = material.metallicFactor;
        hit.roughness = max(material.roughnessFactor, nbl_ROUGHNESS_MIN);
    }

    return hit;
}

void computeDefaultBasis(const vec3 normal, out vec3 x, out vec3 y)
{
    // ZAP's default coordinate system for compatibility
    vec3        z  = normal;
    const float yz = -z.y * z.z;
    y = normalize(((abs(z.z) > 0.99999f) ? vec3(-z.x * z.y, 1.0f - z.y * z.y, yz) : vec3(-z.x * z.z, yz, 1.0f - z.z * z.z)));

    x = cross(y, z);
}

float castShadow(vec3 origin, vec3 direction, float tMin, float tMax)
{
    rayQueryEXT ray_query;
    rayQueryInitializeEXT(ray_query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT, 0xFF, origin, 0.0, direction, tMax);

    while (rayQueryProceedEXT(ray_query)) {}
    if (rayQueryGetIntersectionTypeEXT(ray_query, true) != gl_RayQueryCommittedIntersectionNoneEXT)
    {
        return shadowFactor;
    }
    return 1.0;
}

vec3 computeAmbient(vec3 albedo, float metallic)
{
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    return ambientFactor * mix(albedo, F0, metallic);
}

vec3 computeDirectLighting(vec3 position, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, int forceShadows)
{
    vec3 result = vec3(0.0);

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        GPULightData light = lights.data[i];
        if (light.isEnabled != 1)
        {
            continue;
        }

        vec3  L;
        vec3  radiance;
        float tMax;

        if (light.lightType == LIGHT_POINT)
        {
            vec3  lightDir = light.vector.xyz - position;
            float dist     = length(lightDir);
            L = normalize(lightDir);

            float attenuation = light.intensity / max(15.0, dist * dist);
            radiance = light.color.rgb * attenuation;
            tMax = dist;
        }
        else if (light.lightType == LIGHT_DIRECTIONAL)
        {
            L = normalize(light.vector.xyz);
            radiance = light.color.rgb * light.intensity;
            tMax = 4096.0;
        }
        else
        {
            continue;
        }

        float NdotL = max(0.0, dot(N, L));
        if (NdotL <= 0.0)
        {
            continue;
        }

        vec3 contrib = evaluateBRDF(N, V, L, albedo, metallic, roughness) * radiance;

        #ifdef nbl_RT
        bool wantsShadow = (forceShadows == 1) || (shadowsEnabled != 0 && light.castsShadows == 1);
        if (wantsShadow)
        {
            vec3 origin = offsetRay(position, N);
            contrib *= castShadow(origin, L, 0.01, tMax);
        }
        #endif

        result += contrib;
    }

    return result;
}

vec3 computeIndirectLighting(vec3 position, vec3 N, vec3 albedo, uint seed)
{
    const int SAMPLES = sampleCount;
    vec3  accumulated = vec3(0.0);
    // float hitTSum     = 0.0;
    // int   hitCount    = 0;

    vec3 tangent, bitangent;
    computeDefaultBasis(N, tangent, bitangent);

    vec3 origin = offsetRay(position, N);

    for (int s = 0; s < SAMPLES; s++)
    {
        // Cosine sampling
        float r1        = rnd(seed);
        float r2        = rnd(seed);
        float sq        = sqrt(1.0 - r2);
        float phi       = 2 * PI * r1;
        vec3  direction = vec3(cos(phi) * sq, sin(phi) * sq, sqrt(r2));
        direction       = direction.x * tangent + direction.y * bitangent + direction.z * N;

        rayQueryEXT rq;
        rayQueryInitializeEXT(rq, topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF, origin, 0.01, direction, 65536.0);
        while (rayQueryProceedEXT(rq)) {}

        if (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionTriangleEXT)
        {
            accumulated += nbl_TEMP_SKY_COLOR;
            continue;
        }

        HitSurface hit = reconstructHitSurface(rq);
        vec3 hitV = -direction;
        vec3 Li = hit.emissive + computeDirectLighting(hit.position, hit.normal, hitV, hit.albedo, hit.metallic, hit.roughness, 1);

        accumulated += Li;
        // hitTSum     += rayQueryGetIntersectionTEXT(rq, true);
        // hitCount    += 1;
    }

    // float hitT = hitCount > 0 ? hitTSum / float(hitCount) : 0.0;
    return albedo * (accumulated / float(SAMPLES));
}

// Kaplanyan/Karis normal-filtering — widen roughness where normals vary fast
float normalFiltering(vec3 normal, float roughness)
{
    vec3 dndu = dFdx(normal);
    vec3 dndv = dFdy(normal);
    float variance = 0.5 * (dot(dndu, dndu) + dot(dndv, dndv));
    float kernelRoughness2 = min(2.0 * variance, 0.25);
    float filteredRoughness2 = clamp(roughness * roughness + kernelRoughness2, 0.0, 1.0);
    return sqrt(filteredRoughness2);
}

void main()
{
    float viewZ = texture(uViewZ, inUV).r;

    // Check if there's any geometry at all and render sky
    if (viewZ >= 0.0)
    {
        outColor = vec4(nbl_TEMP_SKY_COLOR, 1.0);
        return;
    }

    uint seed = tea(uint(gl_FragCoord.x), uint(gl_FragCoord.y));

    vec3 worldPosition   = texture(uWorldPosition,  inUV).xyz;
    vec3 worldNormal     = texture(uWorldNormal,    inUV).xyz;
    vec3 albedo          = texture(uAlbedo,         inUV).rgb;

    vec2  lightingParams = texture(uLightingParams, inUV).rg;
    float metallic       = lightingParams.r;
    float roughness      = max(lightingParams.g, nbl_ROUGHNESS_MIN);

    vec3 N = normalize(worldNormal);
    vec3 V = normalize(camera.data.eye.xyz - worldPosition);

    roughness = normalFiltering(N, roughness);

    float aoFactor = 1.0;

    #ifdef nbl_AMBIENT_OCCLUSION
    aoFactor = texture(uAO, inUV).r;
    #endif

    vec3  ambient  = (enableGI == 1) ? vec3(0.0) : computeAmbient(albedo, metallic) * aoFactor;
    vec3  direct   = computeDirectLighting(worldPosition, N, V, albedo, metallic, roughness, 0);
    vec3  indirect = vec3(0.0);

    #ifdef nbl_RT
    if (enableGI == 1)
    {
        indirect = computeIndirectLighting(worldPosition, N, albedo, seed);
    }
    #endif

    outColor = vec4(ambient + direct + indirect, 1.0);
}
