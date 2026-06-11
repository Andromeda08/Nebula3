#version 460

layout(location = 0) in  vec2 inUV;
layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D uDst;

layout(push_constant) uniform PushConstants {
    vec4  srcColor;
    float progress;
};

void main()
{
    vec3 s = texture(uDst, inUV).rgb;
    outColor = vec4(mix(srcColor.rgb, s, progress), 1.0);
}
