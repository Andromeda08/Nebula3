#version 460

// Define this macro to enable ray tracing features.
#define nbl_RAY_TRACING

#extension GL_GOOGLE_include_directive : require

#ifdef nbl_RAY_TRACING
    #extension GL_EXT_ray_tracing : require
    #extension GL_EXT_ray_query : enable
#endif

#include "inc/Nebula.inc.glsl"
#include "inc/PBR.inc.glsl"

layout (location = 0) in  vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform CameraUniform {
    CameraData camera;
};

layout (set = 0, binding = 1) readonly buffer LightUniform {
    GPULightData lights[MAX_LIGHTS];
};

#ifdef nbl_RAY_TRACING
layout (set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;
#endif

layout (set = 1, binding = 0) uniform sampler2D   uViewPositionDepth;
layout (set = 1, binding = 1) uniform sampler2D   uViewNormal;
layout (set = 1, binding = 2) uniform sampler2D   uAlbedoClip;
layout (set = 1, binding = 3) uniform sampler2D   uAmbientOcclusion;
layout (set = 1, binding = 4) uniform sampler2D   uPBRParams;
layout (set = 1, binding = 5) uniform samplerCube uSkyCubeMap;
layout (set = 1, binding = 6) readonly buffer SkyDataBuffer {
    vec4  sunTransmittance;
    vec3  sunDirection;
    float sunIntensity;
};

layout (push_constant) uniform PushConstants {
    int shadowMode;
};

/**
 * Shadows with ray queries.
 */
#ifdef nbl_RAY_TRACING
float castShadow(vec3 origin, vec3 direction, float tMin, float tMax)
{
    const float shadowFactor = 0.1;

    rayQueryEXT ray_query;
    rayQueryInitializeEXT(ray_query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT, 0xFF, origin, tMin, direction, tMax);

    while(rayQueryProceedEXT(ray_query)) {}
    if (rayQueryGetIntersectionTypeEXT(ray_query, true) != gl_RayQueryCommittedIntersectionNoneEXT)
    {
        return shadowFactor;
    }
    return 1.0;
}
#endif

void main()
{
    vec3 viewPosition = texture(uViewPositionDepth, inUV).xyz;

    // Check if there's any geometry at all and render sky
    if (viewPosition.z >= 0.0)
    {
        vec4 clip = vec4(inUV * 2.0 - 1.0, 1.0, 1.0);
        vec4 world = camera.viewInverse * camera.projInverse * clip;
        vec3 rayDir = normalize(world.xyz / world.w - camera.eye.xyz);

        vec3 sky = texture(uSkyCubeMap, rayDir).rgb;

        float sunAngle = acos(clamp(dot(rayDir, sunDirection), -1.0, 1.0));
        if (sunAngle < 0.0047)
        sky += sunTransmittance.rgb * sunIntensity;

        outColor = vec4(sky, 1.0);
        return;
    }

    vec3 viewNormal     = texture(uViewNormal, inUV).rgb;
    vec3 albedo         = texture(uAlbedoClip, inUV).rgb;
    vec4 lightingParams = texture(uPBRParams,  inUV);

    float metallic      = lightingParams.r;
    float roughness     = max(lightingParams.g, 0.089);
    vec3  worldPosition = viewToWorldPosition(viewPosition, camera.viewInverse);
    vec3  worldNormal   = viewToWorldNormal(viewNormal, camera.viewInverse);

    vec3 N = normalize(worldNormal);
    vec3 V = normalize(camera.eye.xyz - worldPosition);

    // Kaplanyan/Karis normal-filtering — widen roughness where normals vary fast
    vec3 dndu = dFdx(N);
    vec3 dndv = dFdy(N);
    float variance = 0.5 * (dot(dndu, dndu) + dot(dndv, dndv));
    float kernelRoughness2 = min(2.0 * variance, 0.25);
    float filteredRoughness2 = clamp(roughness * roughness + kernelRoughness2, 0.0, 1.0);
    roughness = sqrt(filteredRoughness2);

    vec3 finalColor = vec3(0.0);

    // Ambient (Todo: IBL)
    vec3 F0       = mix(vec3(0.04), albedo, metallic);
    vec3 ambient  = 0.15 * mix(albedo, F0, metallic);
    finalColor   += ambient;

    // Sun (Directional Light)
    {
        vec3 L           = sunDirection;
        vec3 sunRadiance = sunTransmittance.rgb * sunIntensity;
        vec3 sunContrib  = evaluateBRDF(N, V, L, albedo, metallic, roughness) * sunRadiance;

        #ifdef nbl_RAY_TRACING
        float NdotL = max(0.0, dot(N, L));
        if (shadowMode != 0 && NdotL > 0.0)
        {
            sunContrib *= castShadow(worldPosition, L, 0.01, 1000.0);
        }
        #endif

        finalColor += sunContrib;
    }

    // Point Lights
    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled != 1)
        {
            continue;
        }

        GPULightData light    = lights[i];
        vec3         lightDir = light.vector.xyz - worldPosition;
        float        dist     = length(lightDir);
        vec3         L        = normalize(lightDir);
        float        NdotL    = max(0.0, dot(N, L));

        float attenuation = light.intensity / max(15.0, dist * dist);
        vec3  radiance    = light.color.rgb * attenuation;
        vec3  contrib     = evaluateBRDF(N, V, L, albedo, metallic, roughness) * radiance;

        #ifdef nbl_RAY_TRACING
        if (shadowMode != 0 && light.castsShadow == 1)
        {
            contrib *= castShadow(worldPosition, L, 0.01, dist);
        }
        #endif

        finalColor += contrib;
    }

    float ambientOcclusionFactor = texture(uAmbientOcclusion, inUV).r;
    outColor = vec4(finalColor * ambientOcclusionFactor, 1.0);
}
