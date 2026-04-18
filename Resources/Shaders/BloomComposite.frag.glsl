#version 460

layout (location = 0) in  vec2 inUV;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D uScene;
layout (set = 0, binding = 1) uniform sampler2D uBloom;

void main()
{
    vec3 scene = texture(uScene, inUV).rgb;
    vec3 bloom = texture(uBloom, inUV).rgb;
    outColor = vec4(scene + bloom * 2.0, 1.0);
}