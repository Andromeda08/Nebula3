#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable

const int GROUP_SIZE = 16;

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE) in;

// Bound Resources
// ========================================
layout(set = 0, binding = 0, rgba32f) uniform image2D inPosition;
layout(set = 0, binding = 1, rgba32f) uniform image2D inNormal;
layout(set = 0, binding = 2, r32f)    uniform image2D outImage;

layout (set = 1, binding = 0) uniform CameraUniform {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    float nearPlane;
    float farPlane;
} camera;

layout(set = 1, binding = 2) uniform accelerationStructureEXT topLevelAS;

layout(push_constant) uniform AoControl {
    float aoRadius;
    int   aoSamples;
    float aoPower;
    int   aoDistanceBased;
    int   frameNumber;
};

//-------------------------------------------------------------------------------------------------
// Avoiding self intersections (see Ray Tracing Gems, Ch. 6)
//
vec3 OffsetRay(in vec3 p, in vec3 n)
{
    const float intScale   = 256.0f;
    const float floatScale = 1.0f / 65536.0f;
    const float origin     = 1.0f / 32.0f;

    ivec3 of_i = ivec3(intScale * n.x, intScale * n.y, intScale * n.z);

    vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
    intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
    intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

    return vec3(abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,  //
    abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,  //
    abs(p.z) < origin ? p.z + floatScale * n.z : p_i.z);
}


//////////////////////////// AO //////////////////////////////////////
#define EPS 0.05
const float M_PI = 3.141592653589;

void ComputeDefaultBasis(const vec3 normal, out vec3 x, out vec3 y)
{
    // ZAP's default coordinate system for compatibility
    vec3        z  = normal;
    const float yz = -z.y * z.z;
    y = normalize(((abs(z.z) > 0.99999f) ? vec3(-z.x * z.y, 1.0f - z.y * z.y, yz) : vec3(-z.x * z.z, yz, 1.0f - z.z * z.z)));

    x = cross(y, z);
}

//-------------------------------------------------------------------------------------------------
// Random
//-------------------------------------------------------------------------------------------------


// Generate a random unsigned int from two unsigned int values, using 16 pairs
// of rounds of the Tiny Encryption Algorithm. See Zafar, Olano, and Curtis,
// "GPU Random Numbers via the Tiny Encryption Algorithm"
uint tea(uint val0, uint val1)
{
    uint v0 = val0;
    uint v1 = val1;
    uint s0 = 0;

    for(uint n = 0; n < 16; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }

    return v0;
}

uvec2 pcg2d(uvec2 v)
{
    v = v * 1664525u + 1013904223u;

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v >> 16u);

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v >> 16u);

    return v;
}

// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
uint lcg(inout uint prev)
{
    uint LCG_A = 1664525u;
    uint LCG_C = 1013904223u;
    prev       = (LCG_A * prev + LCG_C);
    return prev & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint seed)
{
    return (float(lcg(seed)) / float(0x01000000));
}

float TraceRay(in rayQueryEXT rayQuery, in vec3 origin, in vec3 direction)
{
    uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT;
    rayQueryInitializeEXT(rayQuery, topLevelAS, flags, 0xFF, origin, 0.0f, direction, aoRadius);
    while(rayQueryProceedEXT(rayQuery)) {}

    if(rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT)
    {
        if(aoDistanceBased == 0)
        {
            return 1;
        }
        float length = 1 - (rayQueryGetIntersectionTEXT(rayQuery, true) / aoRadius);
        return length;
    }

    return 0;
}


void main()
{
    float occlusion = 0.0;

    ivec2 size = imageSize(inPosition);
    if(gl_GlobalInvocationID.x >= size.x || gl_GlobalInvocationID.y >= size.y) {
        return;
    }

    uint seed = tea(size.x * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x, 0 /* frameNumber */);

    vec3 viewPosition = imageLoad(inPosition, ivec2(gl_GlobalInvocationID.xy)).xyz;
    vec3 viewNormal   = imageLoad(inNormal, ivec2(gl_GlobalInvocationID.xy)).xyz;

    vec3 position = (camera.viewInverse * vec4(viewPosition, 1.0)).xyz;
    vec3 normal   = mat3(camera.viewInverse) * viewNormal;

    // Shooting rays only if a fragment was rendered
    if(position != vec3(0))
    {
        vec3 origin = position;
        vec3 normal = normalize(normal);
        vec3 direction;

        // Move origin slightly away from the surface to avoid self-occlusion
        origin = OffsetRay(origin, normal);

        // Finding the basis (tangent and bitangent) from the normal
        vec3 n, tangent, bitangent;
        ComputeDefaultBasis(normal, tangent, bitangent);

        // Sampling hemiphere n-time
        for(int i = 0; i < aoSamples; i++)
        {
            // Cosine sampling
            float r1        = rnd(seed);
            float r2        = rnd(seed);
            float sq        = sqrt(1.0 - r2);
            float phi       = 2 * M_PI * r1;
            vec3  direction = vec3(cos(phi) * sq, sin(phi) * sq, sqrt(r2));
            direction       = direction.x * tangent + direction.y * bitangent + direction.z * normal;

            rayQueryEXT rayQuery;
            occlusion += TraceRay(rayQuery, origin, direction);
        }

        // Computing occlusion
        occlusion = 1 - (occlusion / aoSamples);
        occlusion = pow(clamp(occlusion, 0, 1), aoPower);
    }

    imageStore(outImage, ivec2(gl_GlobalInvocationID.xy), vec4(occlusion));
}
