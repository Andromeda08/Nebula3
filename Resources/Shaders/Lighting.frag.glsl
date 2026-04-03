#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : enable

#define MAX_LIGHTS 256

struct GPULightData
{
    vec3  vector;
    int   lightType;
    vec3  color;
    float intensity;
    float radius;
    int   enabled;
    int   castsShadow;
    int   _p0;
};
// Input Attributes
// ========================================
layout (location = 0) in vec2 inUV;

// Output Attributes
// ========================================
layout (location = 0) out vec4 outColor;

// Bound Resources
// ========================================
layout (set = 0, binding = 0) uniform CameraUniform {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    float nearPlane;
    float farPlane;
} camera;

layout (set = 0, binding = 1) readonly buffer LightUniform {
    GPULightData data[MAX_LIGHTS];
} lights;

layout (set = 0, binding = 2) uniform accelerationStructureEXT topLevelAS;

layout (set = 1, binding = 0) uniform sampler2D   uPositionDepth;
layout (set = 1, binding = 1) uniform sampler2D   uNormal;
layout (set = 1, binding = 2) uniform sampler2D   uAlbedo;
layout (set = 1, binding = 3) uniform sampler2D   uSSAO;
layout (set = 1, binding = 4) uniform samplerCube uSkyTexture;
layout (set = 1, binding = 5) readonly buffer SkyData {
    vec4  sunTransmittance;
    vec3  sunDirection;
    float sunIntensity;
};

layout (push_constant) uniform PushConstant {
    int shadowMode;
};

// Building an Orthonormal Basis, Revisited, Tom Duff et al. 2017
void branchlessONB(vec3 n, out vec3 b1, out vec3 b2)
{
    const float s = n.z >= 0.0 ? 1.0 : -1.0;
    const float a = -1.0f / (s + n.z);
    const float b = n.x * n.y * a;
    b1 = vec3(1.0 + s * n.x * n.x * a, s * b, -s * n.x);
    b2 = vec3(b, s + n.y * n.y * a, -n.y);
}

vec2 randomDisk(int i, int numSamples)
{
    float noise = fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453);

    float r = sqrt((float(i) + noise) / float(numSamples));
    float theta = 2.39996323 * float(i);

    return vec2(r * cos(theta), r * sin(theta));
}

// Return shadowFactor or 1.0f
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

float castSoftShadow(vec3 origin, GPULightData light, float r)
{
    float shadow = 0.0;
    const int samples = 16;

    vec3 lightDir = light.vector.xyz - origin;
    vec3 b1, b2;
    branchlessONB(lightDir, b1, b2);

    for (int i = 0; i < samples; i++)
    {
        vec2 rnd = randomDisk(i, samples);
        vec3 jitteredLightPos = light.vector.xyz + r * (rnd.x * b1 + rnd.y * b2);

        vec3  dir  = jitteredLightPos - origin.xyz;
        vec3  L    = normalize(dir);
        float tMax = length(dir);

        shadow += castShadow(origin, dir, 0.01, tMax);
    }

    return shadow / float(samples);
}

void main()
{
    vec2 uv = inUV;

    vec3 viewPos = texture(uPositionDepth, uv).rgb;
    if (viewPos.z >= 0.0)
    {
        vec4 clip = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
        vec4 world = camera.viewInverse * camera.projInverse * clip;
        vec3 rayDir = normalize(world.xyz / world.w - camera.eye.xyz);

        vec3 sky = texture(uSkyTexture, rayDir).rgb;

        float sunAngle = acos(clamp(dot(rayDir, sunDirection), -1.0, 1.0));
        if (sunAngle < 0.0047)
        sky += sunTransmittance.rgb * sunIntensity;

        outColor = vec4(sky, 1.0);
        return;
    }

    vec3 viewNormal = texture(uNormal, uv).rgb;
    vec3 albedo     = texture(uAlbedo, uv).rgb;

    vec3 wPos    = (camera.viewInverse * vec4(viewPos, 1.0)).xyz;
    vec3 wNormal = mat3(camera.viewInverse) * viewNormal;

    vec3 N = normalize(wNormal);
    vec3 V = normalize(camera.eye.xyz - wPos);

    vec3 finalColor = vec3(0.0);

    vec3 ambient = 0.05 * albedo;
    finalColor += ambient;

    // sun directional light
    vec3 sunColor = sunTransmittance.rgb * sunIntensity;
    float sunNdotL = max(0.0, dot(N, sunDirection));
    vec3 sunContrib = albedo * sunNdotL * sunColor;

    if (shadowMode != 0 && sunNdotL > 0.0)
    {
        sunContrib *= castShadow(wPos, sunDirection, 0.01, 1000.0);
    }

    finalColor += sunContrib;

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights.data[i].enabled != 1)
        {
            continue;
        }

        GPULightData light = lights.data[i];

        vec3  lightDir = light.vector.xyz - wPos;
        float dist     = length(lightDir);
        vec3  L        = normalize(lightDir);

        float dotNL = max(0.0, dot(N, L));

        vec4 color = vec4(albedo * dotNL, 1.0);
        color.rgb = color.rgb * (light.color.rgb * light.intensity) / max(15.0, (dist * dist));

        if (dotNL <= 0.0)
        {
            continue;
        }

        if (shadowMode != 0 && light.castsShadow == 1)
        {
            vec3 origin = wPos;
            vec3 direction = L;
            float tMin = 0.01;
            float tMax = length(lightDir);

            if (shadowMode == 1)
            {
                color *= castShadow(origin, direction, tMin, tMax);
            }
            if (shadowMode == 2)
            {
                color *= castSoftShadow(origin, light, 1.0);
            }
        }

        finalColor += color.rgb;
    }

    float ssao = texture(uSSAO, uv).r;

    outColor = vec4(finalColor * vec3(ssao), 1.0);
}