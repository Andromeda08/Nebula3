#version 460

layout(location = 0) in  vec2 inUV;
layout(location = 0) out vec4 outColor;

#define MAX_TEXTURES          1024
#define TEXTURE_VALID         1
#define MISSING_TEXTURE_INDEX 0

layout (set = 0, binding = 0)       uniform          sampler2D uTextures[MAX_TEXTURES];
layout (set = 0, binding = 1, r32i) readonly uniform iimage2D  uTextureMeta;

layout(push_constant) uniform PushConstants {
    mat4  proj;
    vec4  color;
    int   textureIndex;
    int   isText;
};

void main()
{
    vec4 finalColor = color;

    vec4 s = texture(uTextures[textureIndex], inUV);
    finalColor = vec4(color.rgb, color.a * s.a);

    outColor = finalColor;
}
