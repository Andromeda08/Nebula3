#version 460

#define SSAO_KERNEL_SIZE 32
#define SSAO_RADIUS      1.0

// Input Attributes
// ========================================
layout (location = 0) in vec2 inUV;

// Output Attributes
// ========================================
layout (location = 0) out float outColor;

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

layout (set = 1, binding = 0) uniform SSAOKernel {
    vec4 samples[SSAO_KERNEL_SIZE];
} SSAO_Kernel;

layout (set = 1, binding = 1) uniform sampler2D SSAO_Noise;
layout (set = 1, binding = 2) uniform sampler2D uPositionDepth;
layout (set = 1, binding = 3) uniform sampler2D uNormal;

void main()
{
    vec2 uv = inUV;
    // uv.y = 1.0 - uv.y;

    vec3 fragPos = texture(uPositionDepth, uv).rgb;
    if (fragPos.z >= 0.0)
    {
        outColor = 1.0;
        return;
    }

    vec3 normal = normalize(texture(uNormal, uv).rgb);

    ivec2 texDim = textureSize(uPositionDepth, 0);
    ivec2 noiseDim = textureSize(SSAO_Noise, 0);
    vec2 noiseScale = vec2(float(texDim.x) / float(noiseDim.x), float(texDim.y) / float(noiseDim.y));
    vec3 randomVec = texture(SSAO_Noise, uv * noiseScale).xyz * 2.0 - 1.0;

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    const float bias = 0.025;

    for (int i = 0; i < SSAO_KERNEL_SIZE; i++)
    {
        vec3 samplePos = fragPos + TBN * SSAO_Kernel.samples[i].xyz * SSAO_RADIUS;

        vec4 offset = camera.proj * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        vec2 sampleUV = clamp(offset.xy, 0.0, 1.0);
        float sampleDepth = texture(uPositionDepth, sampleUV).z;

        float rangeCheck = smoothstep(0.0, 1.0, SSAO_RADIUS / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth > samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    outColor = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));
}