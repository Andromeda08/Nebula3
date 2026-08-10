#pragma once

#include <filesystem>
#include <map>
#include <vector>
#include <fastgltf/types.hpp>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include "Level/Level.hpp"
#include "Level/Geometry/Geometry.hpp"
#include "Level/Geometry/GeometrySystem.hpp"
#include "Level/Material/MaterialPool.hpp"
#include "../TextureManager.hpp"

namespace nbl
{
    namespace detail
    {
        [[nodiscard]] constexpr vk::SamplerAddressMode wrapToVulkan(const fastgltf::Wrap wrap) noexcept
        {
            using enum fastgltf::Wrap;
            using enum vk::SamplerAddressMode;
            switch (wrap)
            {
                case ClampToEdge:    return eClampToEdge;
                case MirroredRepeat: return eMirroredRepeat;
                case Repeat:         return eRepeat;
            }
            return eRepeat;
        }

        [[nodiscard]] constexpr vk::Filter filterToVulkan(const fastgltf::Filter filter) noexcept
        {
            using enum fastgltf::Filter;
            using enum vk::Filter;
            switch (filter)
            {
                case Nearest: return eNearest;
                case Linear:  return eLinear;
                default: {
                    return eLinear;
                }
            }
        }
    }

    struct GLTFLoaderParams
    {
        std::filesystem::path   filePath;

        Level*                  pLevel;
        TextureManager*         pTextureManager;
        GeometrySystem*         pGeometrySystem;
        LightSystem*            pLightSystem;
        MaterialSystem*         pMaterialSystem;
    };

    struct PrimInfo
    {
        size_t      meshIndex     = std::numeric_limits<size_t>::max();
        size_t      primIndex     = std::numeric_limits<size_t>::max();

        Geometry*   pGeometry     = nullptr;
        int32_t     geometryIndex = -1;
        PoolHandle  hMaterial     = {};
        std::string name          = {};

        [[nodiscard]] bool isValid() const
        {
            return meshIndex     != std::numeric_limits<size_t>::max()
                && primIndex     != std::numeric_limits<size_t>::max()
                && pGeometry     != nullptr
                && geometryIndex != -1
                && !hMaterial.isNull()
                && !name.empty();
        }
    };

    class GLTFLoader
    {
    public:
        explicit GLTFLoader(const GLTFLoaderParams& params);

        void load();

    private:
        void loadTexturesAndMaterials(fastgltf::Asset& asset);

        void loadGeometry(fastgltf::Asset& asset);

        void createObjects(fastgltf::Asset& asset);

        void processNode(fastgltf::Asset& asset, size_t nodeIndex, glm::mat4 parentModel = glm::mat4(1.0f)) noexcept;

        [[nodiscard]] std::string getTextureName(const fastgltf::Image& image, size_t index) const noexcept;

        [[nodiscard]] std::string getMeshName(const fastgltf::Mesh& mesh, size_t meshIndex, size_t primitiveIndex) const noexcept;

        [[nodiscard]] static glm::mat4 getWorldTransform(std::variant<fastgltf::TRS, fastgltf::math::fmat4x4>& transform, const glm::mat4& parentModel);

        static constexpr auto   sAttrPosition  = "POSITION";
        static constexpr auto   sAttrNormal    = "NORMAL";
        static constexpr auto   sAttrTexCoord0 = "TEXCOORD_0";
        static constexpr auto   sAttrTexCoord1 = "TEXCOORD_1";
        static constexpr auto   sAttrTangent   = "TANGENT";

        std::filesystem::path   mPath;
        std::string             mName;
        Level*                  mLevel;
        TextureManager*         mTextureManager;
        GeometrySystem*         mGeometrySystem;
        LightSystem*            mLightSystem;
        MaterialSystem*         mMaterialSystem;

        int32_t                    mDefaultSampler;

        // GLTF Material Index [->] Material Handle
        PoolHandle                      mDefaultMaterial;
        std::map<size_t, PoolHandle>    mMaterialMap;
        // GLTF Mesh Index [->] List of Primitives for one Mesh
        std::map<size_t, std::vector<PrimInfo>> mMeshMap;
    };
}
