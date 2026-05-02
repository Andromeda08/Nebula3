#pragma once

#include <filesystem>
#include <map>
#include <ranges>
#include <vector>
#include <fastgltf/core.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.hpp>

#include "Core/Ranges.hpp"
#include "Level/Material/MaterialPool.hpp"
#include "Math/DeltaTime.hpp"
#include "Scene/TextureManager.hpp"

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

    class Level;
    // class TextureManager;
    class GeometrySystem;
    class LightSystem;
    class MaterialSystem;

    struct GLTFLoaderParams
    {
        std::filesystem::path   filePath;

        Level*                  pLevel;
        TextureManager*         pTextureManager;
        GeometrySystem*         pGeometrySystem;
        LightSystem*            pLightSystem;
        MaterialSystem*         pMaterialSystem;
    };

    class GLTFLoader
    {
    public:
        explicit GLTFLoader(const GLTFLoaderParams& params)
        {

        }

        void loadMaterials(const fastgltf::Asset& asset)
        {
            auto textureLoadTime = DeltaTime().initialize();

            const auto textureCount = asset.textures.size();

            // GLTF Sampler Index [->] Sampler Create Info
            std::map<size_t, int32_t>    indexToSampler;
            // GLTF Texture Index [->] Texture Slot
            std::map<size_t, int32_t>    indexToSlot;
            std::map<size_t, vk::Format> indexToFormat;

            // Deduce formats, default: RGBA8Unorm
            for (size_t i = 0; i < asset.textures.size(); i++)
            {
                indexToFormat[i] = vk::Format::eR8G8B8A8Unorm;
            }
            for (auto& material : asset.materials)
            {
                if (material.pbrData.baseColorTexture.has_value())
                {
                    const auto idx = material.pbrData.baseColorTexture->textureIndex;
                    indexToFormat[idx] = vk::Format::eR8G8B8A8Srgb;
                }
                if (material.emissiveTexture.has_value())
                {
                    const auto idx = material.emissiveTexture->textureIndex;
                    indexToFormat[idx] = vk::Format::eR8G8B8A8Srgb;
                }
            }

            struct TextureLoadTask
            {
                size_t      gltfIndex   = -1;
                int32_t     samplerSlot = {};
                std::string name;

                // Result
                int32_t     width       = -1;
                int32_t     height      = -1;
                int32_t     channels    = -1;
                uint8_t*    pixels      = nullptr;
            };
            std::vector<TextureLoadTask> tasks(textureCount);

            for (size_t i = 0; i < textureCount; i++)
            {
                const auto& texture = asset.textures[i];
                if (!texture.imageIndex.has_value())
                {
                    continue;
                }

                // If the sampler is not known yet, create one
                if (texture.samplerIndex.has_value() && !indexToSampler.contains(*texture.samplerIndex))
                {
                    const auto& sampler = asset.samplers[*texture.samplerIndex];
                    auto samplerInfo = vk::SamplerCreateInfo()
                        .setAnisotropyEnable(true)
                        .setMaxAnisotropy(16.0f)
                        .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                        .setUnnormalizedCoordinates(false)
                        .setCompareEnable(false)
                        .setCompareOp(vk::CompareOp::eAlways)
                        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                        .setMipLodBias(0.0f)
                        .setAddressModeU(detail::wrapToVulkan(sampler.wrapS))
                        .setAddressModeV(detail::wrapToVulkan(sampler.wrapT))
                        .setAddressModeW(detail::wrapToVulkan(sampler.wrapS))
                        .setMagFilter(vk::Filter::eLinear)
                        .setMinFilter(vk::Filter::eLinear);

                    if (sampler.magFilter.has_value())
                    {
                        samplerInfo.setMagFilter(detail::filterToVulkan(*sampler.magFilter));
                    }
                    if (sampler.minFilter.has_value())
                    {
                        samplerInfo.setMinFilter(detail::filterToVulkan(*sampler.minFilter));
                    }

                    indexToSampler[*texture.samplerIndex] = mTextureManager->createSampler(samplerInfo);
                }

                // Make load task
                const auto& image = asset.images[*texture.imageIndex];
                tasks[i] = {
                    .gltfIndex   = i,
                    .samplerSlot = indexToSampler[*texture.samplerIndex],
                    .name        = getTextureName(image, i),
                    .width       = -1,
                    .height      = -1,
                    .channels    = -1,
                    .pixels      = nullptr,
                };
            }

            // Load textures in parallel
            const uint32_t threadCount = std::min(static_cast<uint32_t>(tasks.size()), std::thread::hardware_concurrency());
            std::vector<std::thread> workers(threadCount);

            // Thread function
            const auto workerFunction = [&](const uint32_t threadIndex) -> void
            {
                for (auto i = threadIndex; i < tasks.size(); i += threadCount)
                {
                    TextureLoadTask& textureInfo = tasks[i];
                    const auto&      image       = asset.images[*asset.textures[textureInfo.gltfIndex].imageIndex];

                    std::visit(fastgltf::visitor {
                        [&](auto&) -> void {},
                        // Load from URI
                        [&](fastgltf::sources::URI& filePath) -> void {
                            const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
                            textureInfo.pixels = stbi_load(path.c_str(), &textureInfo.width, &textureInfo.height, &textureInfo.channels, 4);
                        },
                        // Load from memory : Array
                        [&](fastgltf::sources::Array& array) -> void {
                            textureInfo.pixels = stbi_load_from_memory(
                                reinterpret_cast<const stbi_uc*>(array.bytes.data()), static_cast<int32_t>(array.bytes.size()),
                                &textureInfo.width, &textureInfo.height, &textureInfo.channels, 4);
                        },
                        // Load from memory : Vector
                        [&](fastgltf::sources::Vector& vector) -> void {
                            textureInfo.pixels = stbi_load_from_memory(
                                reinterpret_cast<const stbi_uc*>(vector.bytes.data()), static_cast<int32_t>(vector.bytes.size()),
                                &textureInfo.width, &textureInfo.height, &textureInfo.channels, 4);
                        },
                        // Load from BufferView
                        [&](fastgltf::sources::BufferView& view) -> void {
                            const auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                            auto&       buffer     = asset.buffers[bufferView.bufferIndex];
                            std::visit(fastgltf::visitor {
                                [&](auto&) -> void {},
                                [&](fastgltf::sources::Array& array) -> void {
                                    textureInfo.pixels = stbi_load_from_memory(
                                        reinterpret_cast<const stbi_uc*>(array.bytes.data() + bufferView.byteOffset),
                                        static_cast<int32_t>(bufferView.byteLength),
                                        &textureInfo.width, &textureInfo.height, &textureInfo.channels, 4);
                                },
                                [&](fastgltf::sources::Vector& vector) -> void {
                                    textureInfo.pixels = stbi_load_from_memory(
                                        reinterpret_cast<const stbi_uc*>(vector.bytes.data() + bufferView.byteOffset),
                                        static_cast<int32_t>(bufferView.byteLength),
                                        &textureInfo.width, &textureInfo.height, &textureInfo.channels, 4);
                                },
                            }, buffer.data);
                        }
                    }, image.data);
                }
            };

            for (auto t = 0; t < threadCount; t++)
            {
                workers[t] = std::thread(workerFunction, t);
            }
            for (auto& t : workers)
            {
                t.join();
            }

            // Upload to GPU
            for (auto& completedTask : tasks)
            {
                if (completedTask.pixels)
                {
                    const uint32_t slot = mTextureManager->loadTextureFromMemory(
                        completedTask.name,
                        completedTask.pixels,
                        completedTask.width,
                        completedTask.height,
                        completedTask.samplerSlot,
                        indexToFormat[completedTask.gltfIndex],
                        std::nullopt);
                    stbi_image_free(completedTask.pixels);

                    indexToSlot[completedTask.gltfIndex] = slot;
                }
                else
                {
                    spdlog::warn("Failed to load texture: {}", completedTask.name);
                }
            }

            const std::vector<int32_t> slots = tasks
                | std::views::transform(&TextureLoadTask::gltfIndex)
                // TODO: "size_t" vs "int32_t" thing and generateMipmaps
                | std::views::transform([](const size_t gltfIndex) { return static_cast<int32_t>(gltfIndex); })
                | std::ranges::to<std::vector>();
            mTextureManager->generateMipmaps(slots, vk::Filter::eNearest);

            const auto textureLoadSeconds = textureLoadTime.getDeltaTime();
            spdlog::debug("Loaded {} textures, decode time: {}s", tasks.size(), textureLoadSeconds);

            // Create Materials
            for (const auto& [materialIndex, material] : enumerate(asset.materials))
            {
                const auto& pbr = material.pbrData;

                MaterialData mat = {
                    .solidColor             = glm::make_vec4(&pbr.baseColorFactor[0]),
                    .hTexture               = -1,
                    .hNormalMap             = -1,
                    .hMetallicRoughnessMap  = -1,
                    .pMetallicFactor        = pbr.metallicFactor,
                    .pRoughnessFactor       = pbr.roughnessFactor,
                    .pIsEmissive            = false,
                    .rtHitGroup             = 0,
                };

                if (pbr.baseColorTexture.has_value())
                {
                    const auto gltfTexIdx = static_cast<int32_t>(pbr.baseColorTexture->textureIndex);
                    if (indexToSlot.contains(gltfTexIdx))
                    {
                        mat.hTexture = indexToSlot[gltfTexIdx];
                    }
                }
                if (material.normalTexture.has_value())
                {
                    const auto gltfTexIdx = static_cast<int32_t>(material.normalTexture->textureIndex);
                    if (indexToSlot.contains(gltfTexIdx))
                    {
                        mat.hNormalMap = indexToSlot[gltfTexIdx];
                    }
                }
                if (pbr.metallicRoughnessTexture.has_value())
                {
                    const auto gltfTexIdx = static_cast<int32_t>(pbr.metallicRoughnessTexture->textureIndex);
                    if (indexToSlot.contains(gltfTexIdx))
                    {
                        mat.hMetallicRoughnessMap = indexToSlot[gltfTexIdx];
                    }
                }

                mMaterialMap[materialIndex] = mMaterialSystem->acquire(mat);
            }
        }

    private:
        [[nodiscard]] std::string getTextureName(const fastgltf::Image& image, const size_t index) const noexcept
        {
            return image.name.empty()
                ? fmt::format("tex_{}_{}",mName, index)
                : fmt::format("tex_{}", image.name);
        }

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

        // GLTF Material Index [->] Material Handle
        std::map<size_t, Handle> mMaterialMap;
    };
}
