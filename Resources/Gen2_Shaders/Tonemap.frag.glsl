#version 460

// Input Attributes
// ========================================
layout (location = 0) in vec2 inUV;

// Output Attributes
// ========================================
layout (location = 0) out vec4 outColor;

// Bound Resources
// ========================================
layout (set = 0, binding = 0) uniform sampler2D uColor;

layout (push_constant) uniform PushConstant
{
    float exposure;
};

// Hable 2010, "Filmic Tonemapping Operators"
vec3 tonemap_uncharted2(vec3 x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
    vec3 color = texture(uColor, inUV).rgb;

    vec3 mapped = tonemap_uncharted2(color * exposure);
    vec3 whiteScale = 1.0 / tonemap_uncharted2(vec3(11.2));

    outColor = vec4(mapped * whiteScale, 1.0);
}