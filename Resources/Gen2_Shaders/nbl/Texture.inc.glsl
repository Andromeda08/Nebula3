#ifndef nbl_TEXTURE_INC_GLSL
#define nbl_TEXTURE_INC_GLSL

#ifndef nbl_TEXTURE_SET
    #error nbl_TEXTURE_SET must be defined
#endif

// Texture Constants
// ============================================================
// Maximum number of textures contained in the descriptor array.
#define nbl_MAX_TEXTURES 1024
// Value in the meta texture marking if a texture slot is currently valid or not.
#define nbl_TEXTURE_VALID 1
// Index to default to when an invalid texture index is used.
#define nbl_MISSING_TEXTURE_INDEX 0

// Texture Descriptor Set
// Before including define "nbl_TEXTURE_SET x" to set the set index.
// ============================================================

// Define descriptor set
layout (set = nbl_TEXTURE_SET, binding = 0)                uniform sampler2D uTextures[nbl_MAX_TEXTURES];
layout (set = nbl_TEXTURE_SET, binding = 1, r32i) readonly uniform iimage2D  uTextureValidity;

// Check for texture validity at the specified index
bool isTextureValid(int textureIndex)
{
    int textureValidity = imageLoad(uTextureValidity, ivec2(textureIndex, 0)).r;
    return textureValidity == nbl_TEXTURE_VALID;
}

#endif
