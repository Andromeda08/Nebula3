#version 460

layout (local_size_x = 8, local_size_y = 4, local_size_z = 6) in;

// Bound Resources
// ========================================
layout (set = 0, binding = 0, rgba16f) writeonly uniform imageCube uSkyTexture;
layout (set = 0, binding = 1) buffer SunData {
    vec4  transmittance;
    vec3  sunDir;
    float sunInt;
};

layout (push_constant) uniform SkyParams {
    vec3  sunDirection;
    float sunIntensity;
};

// Constants
// ========================================
const float PI               = 3.14159265359;
const vec3  betaR            = vec3(5.5e-6, 13.0e-6, 22.4e-6);
const float scaleHeightR     = 8500.0;
const float betaM            = 21e-6;
const float scaleHeightM     = 1200.0;
const float mieG             = 0.758;
const float planetRadius     = 6371e3;
const float atmosphereRadius = 6471e3;
const int   NUM_STEPS        = 32;
const int   NUM_LIGHT_STEPS  = 8;

float rayleighPhase(float cosTheta)
{
    return 3.0 / (16.0 * PI) * (1.0 + cosTheta * cosTheta);
}

float miePhase(float cosTheta, float g)
{
    float g2 = g * g;
    return 3.0 / (8.0 * PI) * ((1.0 - g2) * (1.0 + cosTheta * cosTheta))
        / ((2.0 + g2) * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

float rayIntersect(vec3 origin, vec3 dir)
{
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, origin);
    float c = dot(origin, origin) - atmosphereRadius * atmosphereRadius;
    float d = b * b - 4.0 * a * c;
    if (d < 0.0) return -1.0;
    return (-b + sqrt(d)) / (2.0 * a);
}

void lightMarch(vec3 pos, out float odR, out float odM)
{
    float tMax = rayIntersect(pos, sunDirection);
    float step = tMax / float(NUM_LIGHT_STEPS);
    odR = 0.0;
    odM = 0.0;
    for (int i = 0; i < NUM_LIGHT_STEPS; i++) {
        vec3 p = pos + sunDirection * (step * (float(i) + 0.5));
        float h = length(p) - planetRadius;
        if (h < 0.0) { odR = 1e10; odM = 1e10; return; } // underground
        odR += exp(-h / scaleHeightR) * step;
        odM += exp(-h / scaleHeightM) * step;
    }
}

vec3 atmosphere(vec3 rayDir)
{
    vec3 origin = vec3(0.0, planetRadius + 1.0, 0.0);
    float tMax = rayIntersect(origin, rayDir);
    if (tMax < 0.0) return vec3(0.0);

    float stepSize = tMax / float(NUM_STEPS);
    float odR = 0.0, odM = 0.0;
    vec3 totalR = vec3(0.0);
    vec3 totalM = vec3(0.0);

    for (int i = 0; i < NUM_STEPS; i++)
    {
        vec3 pos = origin + rayDir * (stepSize * (float(i) + 0.5));
        float h = length(pos) - planetRadius;

        float dR = exp(-h / scaleHeightR) * stepSize;
        float dM = exp(-h / scaleHeightM) * stepSize;
        odR += dR;
        odM += dM;

        float sunOdR, sunOdM;
        lightMarch(pos, sunOdR, sunOdM);

        vec3 tau = betaR * (odR + sunOdR) + betaM * (odM + sunOdM);
        vec3 atten = exp(-tau);

        totalR += dR * atten;
        totalM += dM * atten;
    }

    float cosTheta = dot(rayDir, sunDirection);
    return sunIntensity * (totalR * betaR * rayleighPhase(cosTheta)
        + totalM * betaM * miePhase(cosTheta, mieG));
}

vec3 cubeDirection(ivec3 coord, int faceSize)
{
    vec2 uv = (vec2(coord.xy) + 0.5) / float(faceSize) * 2.0 - 1.0;
    switch (coord.z) {
        case 0: return normalize(vec3( 1.0, -uv.y, -uv.x));
        case 1: return normalize(vec3(-1.0, -uv.y,  uv.x));
        case 2: return normalize(vec3( uv.x,  1.0,  uv.y));
        case 3: return normalize(vec3( uv.x, -1.0, -uv.y));
        case 4: return normalize(vec3( uv.x, -uv.y,  1.0));
        case 5: return normalize(vec3(-uv.x, -uv.y, -1.0));
    }
    return vec3(0.0);
}

void main() {
    int faceSize = imageSize(uSkyTexture).x;
    ivec3 coord = ivec3(gl_GlobalInvocationID.xyz);
    if (coord.x >= faceSize || coord.y >= faceSize || coord.z >= 6) return;

    vec3 dir = cubeDirection(coord, faceSize);
    vec3 color = atmosphere(dir);
    imageStore(uSkyTexture, coord, vec4(color, 1.0));

    if (gl_GlobalInvocationID == uvec3(0))
    {
        float odR, odM;
        vec3 origin = vec3(0.0, planetRadius + 1.0, 0.0);
        lightMarch(origin, odR, odM);
        transmittance = vec4(exp(-(betaR * odR + betaM * odM)), 1.0);
        sunDir = sunDirection;
        sunInt = sunIntensity;
    }
}