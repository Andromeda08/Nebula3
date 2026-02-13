#version 460

#define MAX_LIGHTS 100
struct GPULightData
{
    vec4  position;
    vec4  color;
    float intensity;
    int   enabled;
    int   _p0, _p1;
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

layout (set = 1, binding = 0) uniform sampler2D uPositionDepth;
layout (set = 1, binding = 1) uniform sampler2D uNormal;
layout (set = 1, binding = 2) uniform sampler2D uAlbedo;
layout (set = 1, binding = 3) uniform sampler2D uSSAO;

void main()
{
    vec2 uv = inUV;

    vec3 viewPos = texture(uPositionDepth, uv).rgb;
    if (viewPos.z >= 0.0)
    {
        outColor = vec4(0.0);
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

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights.data[i].enabled != 1)
        {
            continue;
        }

        GPULightData light = lights.data[i];

        vec3  lightDir = light.position.xyz - wPos;
        float dist     = length(lightDir);
        vec3  L        = normalize(lightDir);

        float dotNL = max(0.0, dot(N, L));

        vec4 color = vec4(albedo * dotNL, 1.0);
        color.rgb = color.rgb * (light.color.rgb * light.intensity) / (dist * dist);

        finalColor += color.rgb;
    }

    float ssao = texture(uSSAO, uv).r;

    outColor = vec4(finalColor * vec3(ssao), 1.0);
}