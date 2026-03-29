#version 460

layout (local_size_x = 16, local_size_y = 16) in;

layout (set = 0, binding = 0)       uniform sampler2D uAO;
layout (set = 0, binding = 1)       uniform sampler2D uPositionDepth;
layout (set = 0, binding = 2, r32f) uniform image2D   uResult;

layout (push_constant) uniform PushConstants {
    ivec2 direction;
    int   kernelSize;
    float depthSigma;
    float spatialSigma;
};

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size  = imageSize(uResult);
    if (pixel.x >= size.x || pixel.y >= size.y)
    {
        return;
    }

    vec2 uv = (vec2(pixel) + 0.5) / vec2(size);

    float ao    = texture(uAO, uv).r;
    float depth = texture(uPositionDepth, uv).w;

    if (depth == 0.0)
    {
        return;
    }

    float weight  = 1.0;
    float totalAO = ao;

    for (int i = -kernelSize; i <= kernelSize; i++)
    {
        if (i == 0)
        {
            continue;
        }

        ivec2 samplePixel = pixel + direction * i;
        if (samplePixel.x < 0 || samplePixel.x >= size.x || samplePixel.y < 0 || samplePixel.y >= size.y)
        {
            continue;
        }

        vec2  sampleUV    = (vec2(samplePixel) + 0.5) / vec2(size);
        float sampleAO    = texture(uAO, sampleUV).r;
        float sampleDepth = texture(uPositionDepth, sampleUV).w;

        // Spatial weight
        float spatial = exp(-float(i * i) / (2.0 * spatialSigma * spatialSigma));

        // Depth weight
        float dz = abs(depth - sampleDepth);
        float depth = exp(-dz * dz / (2.0 * depthSigma * depthSigma));

        float w = spatial * depth;
        weight  += w;
        totalAO += sampleAO * w;
    }

    imageStore(uResult, pixel, vec4(totalAO / weight));
}
