#version 460

// Input Attributes
// ========================================
layout (location = 0) in vec2 inUV;

// Output Attributes
// ========================================
layout (location = 0) out float outColor;

// Bound Resources
// ========================================
layout (set = 0, binding = 0) uniform sampler2D uSSAO_Buffer;

void main()
{
    vec2 uv = inUV;
    //uv.y = -uv.y;

    const int blurRange = 4;
    int n = 0;
    vec2 texelSize = 1.0 / vec2(textureSize(uSSAO_Buffer, 0));
    float result = 0.0;
    for (int x = -blurRange; x <= blurRange; x++)
    {
        for (int y = -blurRange; y <= blurRange; y++)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(uSSAO_Buffer, uv + offset).r;
            n++;
        }
    }
    outColor = result / (float(n));
}