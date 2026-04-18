#version 460

// Input Attributes
// ========================================
layout (location = 0) in      vec4 inViewPosition;
layout (location = 1) in      vec4 inViewNormal;
layout (location = 2) in      vec2 inUV;
layout (location = 3) in      vec4 inColor;
layout (location = 4) in flat int  inTextureIndex;
layout (location = 5) in flat int  inNormalIndex;
layout (location = 6) in      vec3 inViewTangent;
layout (location = 7) in      vec3 inViewBitangent;

// Output Attributes
// ========================================
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

// Bound Resources
// ========================================
layout (set = 0, binding = 0) uniform CameraUniform {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    vec4  frustumPlanes[6];
    float nearPlane;
    float farPlane;
} camera;

#define MAX_TEXTURES          1024
#define TEXTURE_VALID         1
#define MISSING_TEXTURE_INDEX 0

layout (set = 1, binding = 0)       uniform          sampler2D uTextures[MAX_TEXTURES];
layout (set = 1, binding = 1, r32i) readonly uniform iimage2D  uTextureMeta;

float linearDepth(float depth)
{
    return (camera.nearPlane * camera.farPlane) /
        (camera.farPlane - depth * (camera.farPlane - camera.nearPlane));
}

void main()
{
    vec4 color = inColor;

    if (inTextureIndex >= 0)
    {
        int textureValidity = imageLoad(uTextureMeta, ivec2(inTextureIndex, 0)).r;
        vec4 textureColor = (textureValidity == TEXTURE_VALID)
            ? texture(uTextures[inTextureIndex], inUV)
            : texture(uTextures[MISSING_TEXTURE_INDEX], inUV);
        color = vec4(textureColor.rgb, 1.0);
    }

    outPosition = vec4(inViewPosition.xyz, linearDepth(gl_FragCoord.z));
    outNormal   = vec4(normalize(inViewNormal.xyz), 1.0);
    outAlbedo   = color;
}