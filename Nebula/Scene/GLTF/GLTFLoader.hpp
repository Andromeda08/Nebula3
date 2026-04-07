#pragma once

#include <filesystem>
#include <string>
#include <thread>
#include <unordered_map>

#include <fastgltf/core.hpp>
#include <glm/glm.hpp>

#include "Core/Types.hpp"
#include "Core/Util.hpp"
#include "Math/vec.hpp"
#include "Scene/SceneGeometry.hpp"
#include "VulkanRHI/VulkanCore.hpp"

class LightSystem;
class Geometry;
class TextureManager;
class SceneGeometry;
class SceneV2;

struct GLTFLoaderStats
{
    float ioSeconds          = 0.0f;
    float textureLoadSeconds = 0.0f;
    float meshLoadSeconds    = 0.0f;
    float nodeWalkSeconds    = 0.0f;

    [[nodiscard]] float getTotalTime() const noexcept
    {
        return ioSeconds + textureLoadSeconds + meshLoadSeconds + nodeWalkSeconds;
    }
};

struct GLTFLoaderLoadParams
{
    std::string     fileName;
    TextureManager* pTextureManager;
    SceneGeometry*  pSceneGeometry;
    LightSystem*    pLightSystem;
    SceneV2*        pScene;
};

class GLTFLoader
{
    using TextureSlot      = int32_t;
    using GLTFTextureIndex = int32_t;
    using GLTFMeshIndex    = int32_t;

    struct MeshGeometryInfo
    {
        SPtr<Geometry>  geometry;
        GeometryIndex   geometryIndex;

        glm::vec4       baseColor;
        int32_t         textureIndex;
        int32_t         textureUV;
        int32_t         normalMapIndex;
        int32_t         normalUV;

        nbl::MinMaxResult aabb;
    };

    struct TextureInfo
    {
        GLTFTextureIndex      index       = -1;
        std::string           name        = "tex_unknown";
        unsigned char*        pixels      = nullptr;
        int32_t               width       = -1;
        int32_t               height      = -1;
        int32_t               channels    = -1;
        vk::SamplerCreateInfo samplerInfo = {};
    };
public:
    explicit GLTFLoader(const GLTFLoaderLoadParams& params);

    void load();

    static void loadParts(const GLTFLoaderLoadParams& params, const std::vector<std::string>& files) noexcept;

private:
    void s1_deduceTextureFormats(const fastgltf::Asset& asset) noexcept;

    void s2_parallel_loadTextures(fastgltf::Asset& asset) noexcept;

    void s3_loadMeshes(fastgltf::Asset& asset) noexcept;

    void s4_walkNodes(fastgltf::Asset& asset) noexcept;

    void processNode(fastgltf::Asset& asset, size_t nodeIndex, glm::mat4 parentModel) noexcept;

    // Make texture name -> tex_[<fileName>_<index>, <image.name>]
    [[nodiscard]] std::string getTextureName(const fastgltf::Image& image, uint64_t index) const noexcept;

    [[nodiscard]] static glm::mat4 getWorldTransform(
        std::variant<fastgltf::TRS, fastgltf::math::fmat4x4>& transform, const glm::mat4& parentModel);

    static constexpr auto sAttrPosition  = "POSITION";
    static constexpr auto sAttrNormal    = "NORMAL";
    static constexpr auto sAttrTexCoord0 = "TEXCOORD_0";
    static constexpr auto sAttrTexCoord1 = "TEXCOORD_1";
    static constexpr auto sAttrTangent   = "TANGENT";

    TextureManager* mTextureManager;
    SceneGeometry*  mSceneGeometry;
    LightSystem*    mLightSystem;
    SceneV2*        mScene;

    // Deduced texture format based on Material usages
    std::unordered_map<GLTFTextureIndex, vk::Format>  mTextureFormatMap;
    std::unordered_map<GLTFTextureIndex, TextureSlot> mTextureMap;

    std::unordered_map<GLTFMeshIndex, std::vector<MeshGeometryInfo>> mMeshMap;

    std::filesystem::path   mFileName;
    std::string             mSceneName = "Unknown Scene";

    GLTFLoaderStats mStats = {};
};