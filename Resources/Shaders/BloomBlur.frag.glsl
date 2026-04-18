#version 460

layout (location = 0) in  vec2 inUV;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D uInput;

layout (push_constant) uniform Push {
    vec2 direction;  // (1, 0) for horizontal, (0, 1) for vertical
};

const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(uInput, 0));
    vec3 result = texture(uInput, inUV).rgb * weights[0];

    for (int i = 1; i < 5; i++)
    {
        vec2 offset = direction * texelSize * float(i);
        result += texture(uInput, inUV + offset).rgb * weights[i];
        result += texture(uInput, inUV - offset).rgb * weights[i];
    }

    outColor = vec4(result, 1.0);
}