#include "GLTFLoader.hpp"

#include <ranges>
#include <regex>
#include <thread>
#include <variant>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Core/Ranges.hpp"
#include "Level/Transform.hpp"
#include "Math/DeltaTime.hpp"

namespace nbl
{
    GLTFLoader::GLTFLoader(const GLTFLoaderParams& params)
    : mPath(params.filePath)
    , mName(params.filePath.filename().stem().string())
    , mLevel(params.pLevel)
    , mTextureManager(params.pTextureManager)
    , mGeometrySystem(params.pGeometrySystem)
    , mLightSystem(params.pLightSystem)
    , mMaterialSystem(params.pMaterialSystem)
    {
        mDefaultMaterial = mMaterialSystem->acquire(MaterialData());
    }

    void GLTFLoader::load()
    {
        spdlog::info("Loading Scene: {}", mName);

        // Step 0: Parse and Load GLTF File
        // ======================================
        #pragma region

        auto ioTime = DeltaTime().initialize();

        // Parse File
        fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);
        auto gltfDataBuffer = fastgltf::GltfDataBuffer::FromPath(mPath);
        if (gltfDataBuffer.error() != fastgltf::Error::None)
        {
            exitWithError("Failed to load GLTF file ({}): {}",
                fastgltf::getErrorMessage(gltfDataBuffer.error()), mPath.string());
        }

        // Load Data
        constexpr auto options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::GenerateMeshIndices;
        auto assetResult = parser.loadGltf(gltfDataBuffer.get(), mPath.parent_path(), options);
        if (assetResult.error() != fastgltf::Error::None)
        {
            exitWithError("Failed to parse GLTF ({}): {}",
                fastgltf::getErrorMessage(assetResult.error()), mPath.string());
        }

        const auto ioSeconds = ioTime.getDeltaTime();
        spdlog::info("GLTF IO time: {}s", ioSeconds);

        #pragma endregion

        auto& asset = assetResult.get();

        loadTexturesAndMaterials(asset);
        loadGeometry(asset);
        createObjects(asset);
    }

    void GLTFLoader::loadTexturesAndMaterials(fastgltf::Asset& asset)
    {
        auto textureLoadTime = DeltaTime().initialize();

        const auto textureCount = asset.textures.size();

        // GLTF Sampler Index [->] Sampler Create Info
        std::map<size_t, int32_t>    indexToSampler;
        // GLTF Texture Index [->] Texture Slot
        std::map<size_t, uint32_t>   indexToSlot;
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
            size_t                gltfIndex   = -1;
            vk::SamplerCreateInfo samplerInfo = {};
            std::string           name;

            // Result
            int32_t               width       = -1;
            int32_t               height      = -1;
            int32_t               channels    = -1;
            uint8_t*              pixels      = nullptr;
        };
        std::vector<TextureLoadTask> tasks;
        tasks.reserve(textureCount);

        for (size_t i = 0; i < textureCount; i++)
        {
            const auto& texture = asset.textures[i];
            if (!texture.imageIndex.has_value())
            {
                continue;
            }

            // If the sampler is not known yet, create one
            auto samplerInfo = vk::SamplerCreateInfo()
                .setAnisotropyEnable(true)
                .setMaxAnisotropy(16.0f)
                .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                .setUnnormalizedCoordinates(false)
                .setCompareEnable(false)
                .setCompareOp(vk::CompareOp::eAlways)
                .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                .setMipLodBias(0.0f)
                .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                .setMagFilter(vk::Filter::eLinear)
                .setMinFilter(vk::Filter::eLinear);

            if (texture.samplerIndex.has_value())
            {
                auto& sampler = asset.samplers[*texture.samplerIndex];

                samplerInfo.setAddressModeU(detail::wrapToVulkan(sampler.wrapS));
                samplerInfo.setAddressModeV(detail::wrapToVulkan(sampler.wrapT));
                samplerInfo.setAddressModeW(detail::wrapToVulkan(sampler.wrapS));

                if (sampler.magFilter.has_value())
                {
                    samplerInfo.setMagFilter(detail::filterToVulkan(*sampler.magFilter));
                }
                if (sampler.minFilter.has_value())
                {
                    samplerInfo.setMinFilter(detail::filterToVulkan(*sampler.minFilter));
                }
            }

            // Make load task
            auto& image = asset.images[*texture.imageIndex];
            tasks.push_back({
                .gltfIndex   = i,
                .samplerInfo = samplerInfo,
                .name        = getTextureName(image, i),
                .width       = -1,
                .height      = -1,
                .channels    = -1,
                .pixels      = nullptr,
            });
        }

        // Load textures in parallel
        const uint32_t threadCount = std::min(static_cast<uint32_t>(tasks.size()), std::thread::hardware_concurrency());
        spdlog::info("Decoding textures on {} thread(s)", threadCount);
        std::vector<std::thread> workers(threadCount);

        // Thread function
        const auto workerFunction = [&](const uint32_t threadIndex) -> void
        {
            for (auto i = threadIndex; i < tasks.size(); i += threadCount)
            {
                TextureLoadTask& textureInfo = tasks[i];
                auto&            image       = asset.images[*asset.textures[textureInfo.gltfIndex].imageIndex];

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
        for (auto& textureInfo : tasks)
        {
            if (textureInfo.pixels)
            {
                const uint32_t slot = mTextureManager->loadTextureFromMemory(
                    textureInfo.name, textureInfo.pixels, textureInfo.width, textureInfo.height,
                    std::nullopt, textureInfo.samplerInfo, indexToFormat[textureInfo.gltfIndex]);
                stbi_image_free(textureInfo.pixels);

                indexToSlot[textureInfo.gltfIndex] = slot;
            }
            else
            {
                spdlog::warn("Failed to load texture: {}", textureInfo.name);
            }
        }

        const auto textureLoadSeconds = textureLoadTime.getDeltaTime();
        spdlog::info("Loaded {} textures, decode time: {}s", tasks.size(), textureLoadSeconds);

        // Create Materials
        for (const auto& [materialIndex, material] : enumerate(asset.materials))
        {
            bool isEmissive = false;
            /* Bistro Extras */ if (mName.contains("bistro"))
            {
                const auto name = std::string(material.name);
                static std::regex pattern(R"(^Paris_StringLights_.+_Color_Emissive$)");
                isEmissive = std::regex_match(name, pattern);
            }

            const auto& pbr = material.pbrData;
            MaterialData mat = {
                .solidColor             = glm::make_vec4(&pbr.baseColorFactor[0]),
                .hTexture               = -1,
                .hNormalMap             = -1,
                .hMetallicRoughnessMap  = -1,
                .pMetallicFactor        = pbr.metallicFactor,
                .pRoughnessFactor       = pbr.roughnessFactor,
                .pIsEmissive            = isEmissive,
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

        spdlog::info("Created {} materials", asset.materials.size());
    }

    void GLTFLoader::loadGeometry(fastgltf::Asset& asset)
    {
        for (size_t meshIndex = 0; meshIndex < asset.meshes.size(); meshIndex++)
        {
            const auto& mesh = asset.meshes[meshIndex];

            for (size_t primIndex = 0; primIndex < mesh.primitives.size(); primIndex++)
            {
                auto& prim = mesh.primitives[primIndex];
                if (prim.type != fastgltf::PrimitiveType::Triangles)
                {
                    continue;
                }

                const auto geometryName = getMeshName(mesh, meshIndex, primIndex);

                // Check for position attribute & get vertex count
                const auto* posAccessorIndex = prim.findAttribute(sAttrPosition);
                if (posAccessorIndex == prim.attributes.end())
                {
                    spdlog::warn("Mesh (n={}, i={}) primitive (i={}) contains no position data.", mesh.name, meshIndex, primIndex);
                    continue;
                }

                auto& posAccessor = asset.accessors[posAccessorIndex->accessorIndex];
                const size_t vertexCount = posAccessor.count;

                std::vector<Vertex> vertices(vertexCount);

                // Position
                fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, posAccessor,
                    [&](glm::vec3 pos, const auto i) -> void { vertices[i].position = pos; });

                // Normal
                if (const auto* attr = prim.findAttribute(sAttrNormal); attr != prim.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, asset.accessors[attr->accessorIndex],
                    [&](glm::vec3 n, const auto i) -> void { vertices[i].normal = glm::normalize(n); });
                }

                // Texcoord 0
                if (const auto* attr = prim.findAttribute(sAttrTexCoord0); attr != prim.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(asset, asset.accessors[attr->accessorIndex],
                        [&](glm::vec2 uv0, const auto i) -> void { vertices[i].uv = uv0; });
                }

                // Tangent
                bool hasTangents = false;
                if (const auto* attr = prim.findAttribute(sAttrTangent); attr != prim.attributes.end())
                {
                    hasTangents = true;
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(asset, asset.accessors[attr->accessorIndex],
                        [&](glm::vec4 tangent, const auto i) -> void {
                            const auto& N = vertices[i].normal;
                            auto        T = glm::vec3(tangent);
                            if (glm::dot(T, T) > 0.0f)
                            {
                                T = glm::normalize(T);
                                T = glm::normalize(T - N * glm::dot(N, T));
                            }
                            vertices[i].tangent = glm::vec4(T, (tangent.w > 0.0f) ? 1.0f : -1.0f);
                        });
                }

                // Indices
                std::vector<uint32_t> indices;
                if (prim.indicesAccessor.has_value())
                {
                    auto& indexAccessor = asset.accessors[*prim.indicesAccessor];
                    indices.resize(indexAccessor.count);
                    fastgltf::iterateAccessorWithIndex<uint32_t>(asset, indexAccessor,
                    [&](uint32_t index, const auto i) { indices[i] = index; });
                }

                auto geometry = makeShared<Geometry>(std::move(vertices), std::move(indices), geometryName);

                // Generate tangents if not included in the GLTF data
                if (!hasTangents)
                {
                    geometry->generateTangents();
                }

                const auto geometryIndex = mGeometrySystem->addGeometry(geometry);

                mMeshMap[meshIndex].push_back({
                    .meshIndex     = meshIndex,
                    .primIndex     = primIndex,
                    .pGeometry     = mGeometrySystem->getGeometry(geometryIndex),
                    .geometryIndex = geometryIndex,
                    .hMaterial     = prim.materialIndex.has_value() ? mMaterialMap[*prim.materialIndex] : mDefaultMaterial,
                    .name          = geometryName,
                });
            }
        }
    }

    void GLTFLoader::createObjects(fastgltf::Asset& asset)
    {
        const size_t sceneIndex = asset.defaultScene.has_value() ? *asset.defaultScene : 0;
        const auto&  scene      = asset.scenes[sceneIndex];

        for (const auto nodeIndex : scene.nodeIndices)
        {
            processNode(asset, nodeIndex, glm::mat4(1.0f));
        }
    }

    void GLTFLoader::processNode(fastgltf::Asset& asset, const size_t nodeIndex, glm::mat4 parentModel) noexcept
    {
        auto& node = asset.nodes[nodeIndex];
        const auto worldTransform = getWorldTransform(node.transform, parentModel);

        if (node.meshIndex.has_value())
        {
            for (const auto primInfo : mMeshMap[*node.meshIndex])
            {
                if (!primInfo.isValid())
                {
                    continue;
                }

                mLevel->addObject<Object>({
                    .geometryIndex = primInfo.geometryIndex,
                    .transform = Transform().setCustomMatrix(worldTransform),
                    .hMaterial = primInfo.hMaterial,
                    .name = fmt::format("obj_{}", primInfo.name),
                });
            }
        }

        if (node.lightIndex.has_value())
        {
            const auto& light = asset.lights[*node.lightIndex];
            if (light.type == fastgltf::LightType::Point)
            {
                const auto hLight = mLightSystem->acquire({
                    .vector         = glm::vec3(worldTransform[3]),
                    .color          = glm::vec3(light.color.x(), light.color.y(), light.color.z()),
                    .intensity      = 5000.0f,
                    .isEnabled      = true,
                    .castsShadows   = true,
                    .radius         = 50.0f,
                    .type           = LightType::Point,
                    .name           = fmt::format("light_{}", *node.lightIndex)
                });
            }
        }

        for (const size_t childIndex : node.children)
        {
            processNode(asset, childIndex, worldTransform);
        }
    }

    std::string GLTFLoader::getTextureName(const fastgltf::Image& image, const size_t index) const noexcept
    {
        return image.name.empty()
            ? fmt::format("tex_{}_{}",mName, index)
            : fmt::format("tex_{}", image.name);
    }

    std::string GLTFLoader::getMeshName(const fastgltf::Mesh& mesh, const size_t meshIndex, const size_t primitiveIndex) const noexcept
    {
        return fmt::format("geo_{}_mesh_{}_prim_{}",
            mName,
            (mesh.name.empty() ? std::to_string(meshIndex) : mesh.name.c_str()),
            primitiveIndex);
    }

    glm::mat4 GLTFLoader::getWorldTransform(std::variant<fastgltf::TRS, fastgltf::math::fmat4x4>& transform, const glm::mat4& parentModel)
    {
        glm::mat4 local(1.0f);

        std::visit(fastgltf::visitor {
            [&](fastgltf::math::fmat4x4& matrix) -> void {
                std::memcpy(&local, matrix.data(), sizeof(glm::mat4));

            },
            [&](fastgltf::TRS& trs) -> void {
                glm::vec3 t(trs.translation[0], trs.translation[1], trs.translation[2]);
                glm::quat r(trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]);
                glm::vec3 s(trs.scale[0], trs.scale[1], trs.scale[2]);
                local = glm::translate(glm::mat4(1.0f), t)
                * glm::mat4_cast(r)
                * glm::scale(glm::mat4(1.0f), s);

            },
        }, transform);

        return parentModel * local;
    }
}
