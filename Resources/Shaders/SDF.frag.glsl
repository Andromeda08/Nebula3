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

float distanceFromSphere(in vec3 p, in vec3 c, float r)
{
    return length(p - c) - r;
}

float normLinear01(float v, float min, float max) {
    return (v-min) / (max-min);
}

float sampleSDF(in vec3 p) 
{
    const int W = 100;
    const int H = 100;
    const int D = 100;

    // Position in the texture dimensions
    if (p.x <= W/2 && p.x >= -W/2 &&
        p.y <= H/2 && p.y >= -H/2 &&
        p.z <= D/2 && p.z >= -D/2) 
    {
        vec3 samplePos = vec3(
            normLinear01(p.x, -W/2, W/2),
            normLinear01(p.y, -H/2, H/2),
            normLinear01(p.z, -D/2, D/2)
        );
        return texture(SDFTexture, samplePos).r;
    }
    // Position not in the texture dimensions
    // -> Return distance
    else 
    {
        float rSphereApprox = sqrt(pow(W, 2) + pow(H, 2) + pow(D, 2));
        float d = distanceFromSphere(p, vec3(0.0), rSphereApprox);
        return d;
    }
}

vec4 rayMarch(vec3 ro, vec3 rd)
{
    float total_distance_traveled = 0.0;
    const int NUMBER_OF_STEPS = 512;
    const float MINIMUM_HIT_DISTANCE = 0.01;
    const float MAXIMUM_TRACE_DISTANCE = 1000.0;

    for (int i = 0; i < NUMBER_OF_STEPS; ++i)
    {
        vec3 current_position = ro + total_distance_traveled * rd;

        float distance_to_closest = sampleSDF(current_position);

        if (distance_to_closest < MINIMUM_HIT_DISTANCE) 
        {
            return vec4(1.0, 0.0, 0.0, total_distance_traveled);
        }

        if (total_distance_traveled > MAXIMUM_TRACE_DISTANCE)
        {
            break;
        }
        total_distance_traveled += distance_to_closest;
    }
    return vec4(vec3(0.0), 1.0);
}

void main()
{
    vec4 ro = cameraData.position;

    vec3 rd = (cameraData.projInverse * vec4(inUV /** 2 - 1*/, 0, 1)).xyz;
    rd = (cameraData.viewInverse * vec4(rd, 0)).xyz;
    rd = normalize(rd);

    vec4 rayMarchSample = rayMarch(ro.xyz, rd);

    outColor = vec4(vec3(rayMarchSample), 0.2);
    //outColor = texture(SDFTexture, vec3(inUV, 1.0));
}