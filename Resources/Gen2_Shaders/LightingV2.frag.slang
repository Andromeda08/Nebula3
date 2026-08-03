#language slang 2026

#include "lib/nbl.slang"
#include "lib/CookTorrance.slang"

[[vk::binding(0, 0)]] Sampler2D uWorldPosition;
[[vk::binding(1, 0)]] Sampler2D uWorldNormal;
[[vk::binding(2, 0)]] Sampler2D uAlbedo;
[[vk::binding(3, 0)]] Sampler2D uLightingParams;
[[vk::binding(4, 0)]] Sampler2D uViewZ;
[[vk::binding(5, 0)]] Sampler2D uAO;

struct LightingPushConstants
{
    GPUCameraData* camera;
    GPULightData*  lights;
};

[[vk::push_constant]] LightingPushConstants pc;

#ifdef nbl_RT
[[vk::binding(0, 1)]] RaytracingAccelerationStructure uTopLevelAS;

float3 offsetRay(float3 p, float3 n)
{
    const float intScale   = 256.0;
    const float floatScale = 1.0 / 65536.0;
    const float origin     = 1.0 / 32.0;

    int3 of_i = int3(intScale * n);

    float3 p_i = asfloat(asint(p) + select(p < 0.0, -of_i, of_i));

    return select(abs(p) < origin, p + floatScale * n, p_i);
}

float castShadow(float3 origin, float3 direction, float tMin, float tMax)
{
    RayDesc ray;
    ray.Origin    = origin;
    ray.TMin      = tMin;
    ray.Direction = direction;
    ray.TMax      = tMax;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE> q;
    q.TraceRayInline(uTopLevelAS, RAY_FLAG_NONE, 0xFF, ray);

    while (q.Proceed()) {}

    if (q.CommittedStatus() != COMMITTED_NOTHING)
    {
        return 0.1;
    }

    return 1.0;
}
#endif

float3 computeAmbient(float3 albedo, float metallic)
{
    float3 F0 = lerp(float3(0.04), albedo, metallic);
    return 0.1 * lerp(albedo, F0, metallic);
}

float3 computeDirectLighting(float3 position, float3 N, float3 V, float3 albedo, float metallic, float roughness, int forceShadows)
{
    float3 result = float3(0.0);

    for (int i = 0; i < nbl::MAX_LIGHTS; i++)
    {
        GPULightData light = pc.lights[i];
        if (light.isEnabled != 1)
        {
            continue;
        }

        float3  L;
        float3  radiance;
        float tMax;

        if (light.lightType == nbl::LIGHT_POINT)
        {
            float3  lightDir = light.vector.xyz - position;
            float dist     = length(lightDir);
            L = normalize(lightDir);

            float attenuation = light.intensity / max(15.0, dist * dist);
            radiance = light.color.rgb * attenuation;
            tMax = dist;
        }
        else if (light.lightType == nbl::LIGHT_DIRECTIONAL)
        {
            L = normalize(light.vector.xyz);
            radiance = light.color.rgb * light.intensity;
            tMax = 4096.0;
        }
        else
        {
            continue;
        }

        var NdotL = max(0.0, dot(N, L));
        if (NdotL <= 0.0)
        {
            continue;
        }

        float3 contrib = evaluateBRDF(N, V, L, albedo, metallic, roughness) * radiance;

        #ifdef nbl_RT
        float3 origin = offsetRay(position, N);
        contrib *= castShadow(origin, L, 0.01, tMax);
        #endif

        result += contrib;
    }

    return result;
}

// Kaplanyan/Karis normal-filtering — widen roughness where normals vary fast
float normalFiltering(float3 normal, float roughness)
{
    float3 dndu = ddx(normal);
    float3 dndv = ddy(normal);
    float variance = 0.5 * (dot(dndu, dndu) + dot(dndv, dndv));
    float kernelRoughness2 = min(2.0 * variance, 0.25);
    float filteredRoughness2 = clamp(roughness * roughness + kernelRoughness2, 0.0, 1.0);
    return sqrt(filteredRoughness2);
}

[shader("fragment")]
[require(spvRayQueryKHR)]
float4 main(float2 uv) : SV_Target0
{
    var viewZ = uViewZ.Sample(uv).r;
    if (viewZ >= 0.0)
    {
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    var worldPosition = uWorldPosition.Sample(uv).xyz;
    var worldNormal   = uWorldNormal.Sample(uv).xyz;
    var albedo        = uAlbedo.Sample(uv).xyz;

    var lParams       = uLightingParams.Sample(uv).rg;
    var metallic      = lParams.r;
    var roughness     = max(lParams.g, 0.089);

    float3 N = normalize(worldNormal);
    float3 V = normalize(pc.camera.eye.xyz - worldPosition);

    roughness = normalFiltering(N, roughness);

    var aoFactor = uAO.Sample(uv).r;
    var ambient  = computeAmbient(albedo, metallic) * aoFactor;
    var direct   = computeDirectLighting(worldPosition, N, V, albedo, metallic, roughness, 0);

    return float4(ambient + direct, 1.0);
}
