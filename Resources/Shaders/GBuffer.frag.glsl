#version 460

struct LightParameters
{
    vec4  position;
    vec4  color;
    float intensity;
    int   isActive;
};

#define MAX_TEXTURES          100
#define MAX_LIGHTS            10

#define TEXTURE_VALID         1     // uTextureMeta equals 1 if there's a texture in a certain slot, 0 otherwise.
#define MISSING_TEXTURE_INDEX 0     // Missing texture location index

#define PI 3.14159265

// Input Attributes
layout (location = 0) in      vec4 frWorldPosition;
layout (location = 1) in      vec4 frWorldNormal;
layout (location = 2) in      vec2 frUv;
layout (location = 3) in      vec4 frColor;
layout (location = 4) in flat int  frTextureIndex;

// Output Attributes
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

// Descriptors
layout (set = 0, binding = 0)                uniform sampler2D uTextures[MAX_TEXTURES];
layout (set = 0, binding = 1, r32i) readonly uniform iimage2D  uTextureMeta;

layout (set = 1, binding = 0) uniform LightUniform {
    LightParameters lights[MAX_LIGHTS];
};

layout (set = 1, binding = 1) uniform CameraUniform {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    vec4  frustumPlanes[6];
    float nearPlane;
    float farPlane;
} camera;


void main()
{
    vec4 color = frColor;

    if (frTextureIndex >= 0)
    {
        int textureValidity = imageLoad(uTextureMeta, ivec2(frTextureIndex, 0)).r;
        vec4 textureColor = (textureValidity == TEXTURE_VALID)
            ? texture(uTextures[frTextureIndex], frUv)
            : texture(uTextures[MISSING_TEXTURE_INDEX], frUv);
        color = vec4(textureColor.rgb, 1.0);
    }

    outPosition = frWorldPosition;
    outNormal   = frWorldNormal;
    outAlbedo   = color;
}