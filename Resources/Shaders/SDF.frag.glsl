#version 460

// Input Attributes
// ========================================
layout (location = 0) in vec2 inUV;

// Output Attributes
// ========================================
layout (location = 0) out vec4 outColor;

// Bound Resources
// ========================================
layout (set = 0, binding = 0) uniform CameraData {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  position;
    float nearPlane;
    float farPlane;
    float _p0, _p1, _p3;
} cameraData;

layout (set = 1, binding = 0) uniform sampler3D SDFTexture;

layout (push_constant) uniform Config {
    vec4 bboxMin;
    vec4 bboxMax;
    vec4 sesColor;
    float voxelSize;
    float blending;
    float ls;
    int useSubsurfaceScattering;
    int raymarchingSteps;
} config;

float distanceFromSphere(in vec3 p, in vec3 c, float r)
{
    return length(p - c) - r;
}

float normLinear01(float v, float min, float max) {
    return (v-min) / (max-min);
}

float sampleSDF(in vec3 p) 
{
    vec3 uvw = (p - config.bboxMin.xyz) / (config.bboxMax.xyz - config.bboxMin.xyz);

    if (any(lessThan(uvw, vec3(0.0))) ||
        any(greaterThan(uvw, vec3(1.0))))
    {
        // Outside of texture
        return 1.0;
    }

    return texture(SDFTexture, uvw).r;
}

bool intersectAABB(vec3 ro, vec3 rd, out float tEnter, out float tExit)
{
    vec3 invD = 1.0 / rd;
    vec3 t0 = (config.bboxMin.xyz - ro) * invD;
    vec3 t1 = (config.bboxMax.xyz - ro) * invD;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    tEnter = max(max(tmin.x, tmin.y), tmin.z);
    tExit = min(min(tmax.x, tmax.y), tmax.z);

    return tExit >= max(tEnter, 0.0);
}

vec4 rayMarch(vec3 ro, vec3 rd)
{
    float tEnter, tExit;
    if (!intersectAABB(ro, rd, tEnter, tExit)) return vec4(0.0);
    float t = max(tEnter, 0.0);

    for (int i = 0; i < config.raymarchingSteps && t < tExit; ++i)
    {
        vec3 p = ro + t * rd;
        float d = sampleSDF(p);

        if (d < config.voxelSize) {
            vec4 color = config.sesColor;

            if (config.useSubsurfaceScattering == 1) {
                vec3 subsurfSample = p + config.ls * rd;
                float l = sampleSDF(subsurfSample);
                float s = (config.ls - l) / 2 * config.ls;
                color = clamp(color + vec4(s), 0.0, 1.0);
                /* color += vec4(vec3(s), 0.0);
                float colorMin = min(min(color.x, color.y), color.z);
                float colorMax = max(max(color.x, color.y), color.z);
                color.x = normLinear01(color.x, colorMin, colorMax);
                color.y = normLinear01(color.y, colorMin, colorMax);
                color.z = normLinear01(color.z, colorMin, colorMax); */
            }

            return color;
        }

        t += max(d, config.voxelSize);
    }

    return vec4(0.0);
}

void main()
{
    vec2 ndc = inUV * 2.0 - 1.0;

    vec4 rc = vec4(ndc, -1.0, 1.0);
    vec4 rv = cameraData.projInverse * rc;
    rv = vec4(rv.xyz / rv.w, 0.0);

    vec3 rd = normalize((cameraData.viewInverse * rv).xyz);
    vec3 ro = cameraData.position.xyz;

    vec4 rayMarchSample = rayMarch(ro, rd);

    outColor = vec4(vec3(rayMarchSample), config.blending);
    //outColor = vec4(abs(rd), 1.0);
}