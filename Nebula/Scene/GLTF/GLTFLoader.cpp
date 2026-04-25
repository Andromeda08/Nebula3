#include "GLTFLoader.hpp"

#include <variant>

#include <mikktspace.h>
#include <stb_image.h>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Math/DeltaTime.hpp"
#include "Scene/LightSystem.hpp"
#include "Scene/SceneGeometry.hpp"
#include "Scene/SceneV2.hpp"
#include "Scene/TextureManager.hpp"
#include "Scene/Geometry/Geometry.hpp"
#include "Window/SplashWindow.hpp"

namespace detail
{
    [[nodiscard]] static vk::SamplerAddressMode toVkWrap(const fastgltf::Wrap wrap) noexcept
    {
        switch (wrap)
        {
            case fastgltf::Wrap::ClampToEdge:    return vk::SamplerAddressMode::eClampToEdge;
            case fastgltf::Wrap::MirroredRepeat: return vk::SamplerAddressMode::eMirroredRepeat;
            case fastgltf::Wrap::Repeat:         return vk::SamplerAddressMode::eRepeat;
        }
        return vk::SamplerAddressMode::eRepeat;
    }

    [[nodiscard]] static vk::Filter toVkFilter(const fastgltf::Filter filter) noexcept
    {
        switch (filter)
        {
            case fastgltf::Filter::Nearest: return vk::Filter::eNearest;
            case fastgltf::Filter::Linear:  return vk::Filter::eLinear;
        }
        return vk::Filter::eLinear;
    }
}

namespace tangent
{
    struct MikkTSpaceContext
    {
        std::vector<Vertex>*    vertices;
        std::vector<uint32_t>*  indices;
    };

    static int getNumFaces(const SMikkTSpaceContext* pContext)
    {
        const auto* data = static_cast<MikkTSpaceContext*>(pContext->m_pUserData);
        return static_cast<int>(data->indices->size() / 3);
    }

    static int getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
    {
        return 3;
    }

    static void getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
    {
        const auto*      data  = static_cast<MikkTSpaceContext*>(pContext->m_pUserData);
        const uint32_t   index = (*data->indices)[iFace * 3 + iVert];
        const glm::vec3& pos   = (*data->vertices)[index].position;

        fvPosOut[0] = pos.x;
        fvPosOut[1] = pos.y;
        fvPosOut[2] = pos.z;
    }

    static void getNormal(const SMikkTSpaceContext* pContext, float fvNormalOut[], const int iFace, const int iVert)
    {
        const auto*      data  = static_cast<MikkTSpaceContext*>(pContext->m_pUserData);
        const uint32_t   index = (*data->indices)[iFace * 3 + iVert];
        const glm::vec3& norm  = (*data->vertices)[index].normal;

        fvNormalOut[0] = norm.x;
        fvNormalOut[1] = norm.y;
        fvNormalOut[2] = norm.z;
    }

    static void getTexCoord(const SMikkTSpaceContext* pContext, float fvUVOut[], const int iFace, const int iVert)
    {
        const auto*      data  = static_cast<MikkTSpaceContext*>(pContext->m_pUserData);
        const uint32_t   index = (*data->indices)[iFace * 3 + iVert];
        const glm::vec2& uv = (*data->vertices)[index].uv;

        fvUVOut[0] = uv.x;
        fvUVOut[1] = uv.y;
    }

    static void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
    {
        const auto*      data  = static_cast<MikkTSpaceContext*>(pContext->m_pUserData);
        const uint32_t   index = (*data->indices)[iFace * 3 + iVert];

        Vertex& vertex = (*data->vertices)[index];
        vertex.tangent = {
            fvTangent[0], fvTangent[1], fvTangent[2],
            (fSign >= 0.0f) ? 1.0f : -1.0f,
        };
    }
}

GLTFLoader::GLTFLoader(const GLTFLoaderLoadParams& params)
{
    mFileName  = Configuration::getSceneFilePath(params.fileName);
    mSceneName = mFileName.filename().stem().string();

    mTextureManager = params.pTextureManager;
    mSceneGeometry  = params.pSceneGeometry;
    mLightSystem    = params.pLightSystem;
    mMaterialPool   = params.pMaterialPool;
    mScene          = params.pScene;
}

void GLTFLoader::load()
{
    spdlog::info("Loading scene: {}", mSceneName);

    #pragma region "Step 0: Parse and Load GLTF File"

    auto ioTime = DeltaTime().initialize();

    // Parse File
    fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);
    auto gltfDataBuffer = fastgltf::GltfDataBuffer::FromPath(mFileName);
    if (gltfDataBuffer.error() != fastgltf::Error::None)
    {
        exitWithError("Failed to load GLTF file ({}): {}",
            fastgltf::getErrorMessage(gltfDataBuffer.error()), mFileName.string());
    }

    // Load Data
    constexpr auto options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::GenerateMeshIndices;
    auto assetResult = parser.loadGltf(gltfDataBuffer.get(), mFileName.parent_path(), options);
    if (assetResult.error() != fastgltf::Error::None)
    {
        exitWithError("Failed to parse GLTF ({}): {}",
            fastgltf::getErrorMessage(assetResult.error()), mFileName.string());
    }

    mStats.ioSeconds = ioTime.getDeltaTime();

    #pragma endregion

    spdlog::debug("GLTF IO time: {}s", mStats.ioSeconds);
    auto& asset = assetResult.get();

    s1_deduceTextureFormats(asset);
    s2_parallel_loadTextures(asset);

    {
        auto meshLoadTime = DeltaTime().initialize();
        s3_loadMeshes(asset);
        mStats.meshLoadSeconds = meshLoadTime.getDeltaTime();
        spdlog::debug("Loaded meshes ({}s)", mStats.meshLoadSeconds);
    }

    // Must be called before walking nodes & creation objects
    mSceneGeometry->commit();

    {
        auto nodeWalkTime = DeltaTime().initialize();
        s4_walkNodes(asset);
        mStats.nodeWalkSeconds = nodeWalkTime.getDeltaTime();
        spdlog::debug("Created scene objects ({}s)", mStats.nodeWalkSeconds);
    }

    spdlog::info("Loaded GLTF Scene: {} ({}s)", mSceneName, mStats.getTotalTime());
}

void GLTFLoader::loadParts(const GLTFLoaderLoadParams& params, const std::vector<std::string>& files) noexcept
{
    for (const auto& file : files)
    {
        GLTFLoaderLoadParams localParams = params;
        localParams.fileName = file;
        GLTFLoader loader(localParams);
        loader.load();
    }
}

void GLTFLoader::s1_deduceTextureFormats(const fastgltf::Asset& asset) noexcept
{
    // Default format: RGBA8Unorm
    for (size_t i = 0; i < asset.textures.size(); i++)
    {
        mTextureFormatMap[i] = vk::Format::eR8G8B8A8Unorm;
    }

    for (auto& material : asset.materials)
    {
        if (material.pbrData.baseColorTexture.has_value())
        {
            const auto idx = material.pbrData.baseColorTexture->textureIndex;
            mTextureFormatMap[idx] = vk::Format::eR8G8B8A8Srgb;
        }
        if (material.emissiveTexture.has_value())
        {
            const auto idx = material.emissiveTexture->textureIndex;
            mTextureFormatMap[idx] = vk::Format::eR8G8B8A8Srgb;
        }
    }
}

void GLTFLoader::s2_parallel_loadTextures(fastgltf::Asset& asset) noexcept
{
    auto textureLoadTime = DeltaTime().initialize();

    std::vector<TextureInfo> textureInfos;
    textureInfos.reserve(asset.textures.size());

    // Create work items (texture index, name, sampler config)
    for (GLTFTextureIndex texIndex = 0; texIndex < asset.textures.size(); texIndex++)
    {
        auto& texture = asset.textures[texIndex];
        if (!texture.imageIndex.has_value())
        {
            continue;
        }

        auto& image = asset.images[*texture.imageIndex];

        auto samplerInfo = vk::SamplerCreateInfo()
            .setAnisotropyEnable(true)
            .setMaxAnisotropy(16.0f)
            .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
            .setUnnormalizedCoordinates(false)
            .setCompareEnable(false)
            .setCompareOp(vk::CompareOp::eAlways)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
            .setMipLodBias(0.0f)
            // GLTF Default sampler config
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat)
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear);

        if (texture.samplerIndex.has_value())
        {
            auto& sampler = asset.samplers[*texture.samplerIndex];

            samplerInfo.setAddressModeU(detail::toVkWrap(sampler.wrapS));
            samplerInfo.setAddressModeV(detail::toVkWrap(sampler.wrapT));
            samplerInfo.setAddressModeW(detail::toVkWrap(sampler.wrapS));

            if (sampler.magFilter.has_value())
            {
                samplerInfo.setMagFilter(detail::toVkFilter(*sampler.magFilter));
            }
            if (sampler.minFilter.has_value())
            {
                samplerInfo.setMinFilter(detail::toVkFilter(*sampler.minFilter));
            }
        }

        TextureInfo textureInfo = {
            .index       = texIndex,
            .name        = getTextureName(image, texIndex),
            .samplerInfo = std::move(samplerInfo),
        };

        textureInfos.push_back(std::move(textureInfo));
    }

    // Parallel decode
    const uint32_t threadCount = std::min(static_cast<uint32_t>(textureInfos.size()), std::thread::hardware_concurrency());
    std::vector<std::thread> workers(threadCount);

    // Thread function
    const auto workerFunction = [&](const uint32_t threadIndex) -> void {
        for (auto i = threadIndex; i < textureInfos.size(); i += threadCount)
        {
            auto& textureInfo = textureInfos[i];
            auto& image       = asset.images[*asset.textures[textureInfo.index].imageIndex];

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
    for (auto& textureInfo : textureInfos)
    {
        if (textureInfo.pixels)
        {
            const uint32_t slot = mTextureManager->loadTextureFromMemory(
                textureInfo.name, textureInfo.pixels, textureInfo.width, textureInfo.height,
                std::nullopt, textureInfo.samplerInfo, mTextureFormatMap[textureInfo.index]);
            stbi_image_free(textureInfo.pixels);

            mTextureMap[textureInfo.index] = slot;
            SplashWindow::get().setMessage(std::format("Loaded texture [slot={}]: {}", slot, textureInfo.name), mSceneName);
        }
        else
        {
            spdlog::warn("Failed to load texture: {}", textureInfo.name);
        }
    }

    std::vector<int32_t> slots = textureInfos | std::views::transform(&TextureInfo::index) | std::ranges::to<std::vector>();
    mTextureManager->generateMipmaps(slots, vk::Filter::eNearest);

    mStats.textureLoadSeconds = textureLoadTime.getDeltaTime();
    spdlog::debug("Loaded {} textures, decode time: {}s", textureInfos.size(), mStats.textureLoadSeconds);
}

void GLTFLoader::s3_loadMeshes(fastgltf::Asset& asset) noexcept
{
    for (GLTFMeshIndex meshIndex = 0; meshIndex < asset.meshes.size(); meshIndex++)
    {
        auto& mesh = asset.meshes[meshIndex];

        for (auto primIndex = 0; primIndex < mesh.primitives.size(); primIndex++)
        {
            auto& prim = mesh.primitives[primIndex];
            if (prim.type != fastgltf::PrimitiveType::Triangles)
            {
                continue;
            }

            const std::string geometryName = fmt::format("geo_{}_mesh{}_prim{}", mFileName.string(), meshIndex, primIndex);

            // Check for position attribute & get vertex count
            const auto* posAccessorIndex = prim.findAttribute(sAttrPosition);
            if (posAccessorIndex == prim.attributes.end())
            {
                spdlog::warn("Mesh (n={}, i={}) primitive (i={}) contains no position data.",
                    mesh.name, meshIndex, primIndex);
                continue;
            }

            auto& posAccessor = asset.accessors[posAccessorIndex->accessorIndex];
            const size_t vertexCount = posAccessor.count;

            std::vector<Vertex> vertices(vertexCount);

            // Position
            fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, posAccessor,
                [&](glm::vec3 pos, const auto i) -> void { vertices[i].position = pos; });

            // Compute AABB
            const auto aabbInput = vertices
                | std::views::transform([](const Vertex& v){ return v.position; })
                | std::ranges::to<std::vector>();
            const auto aabb = nbl::BoundingBox::fromPoints(aabbInput);

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

            // Texcoord 1
            if (const auto* attr = prim.findAttribute(sAttrTexCoord1); attr != prim.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec2>(asset, asset.accessors[attr->accessorIndex],
                    [&](glm::vec2 uv1, const auto i) -> void { vertices[i].uv1 = uv1; });
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

            // Generate tangents if not included in the GLTF data
            if (!hasTangents)
            {
                tangent::MikkTSpaceContext nblContext = { &vertices, &indices };

                SMikkTSpaceInterface interface {};
                interface.m_getNumFaces          = tangent::getNumFaces;
                interface.m_getNumVerticesOfFace = tangent::getNumVerticesOfFace;
                interface.m_getPosition          = tangent::getPosition;
                interface.m_getNormal            = tangent::getNormal;
                interface.m_getTexCoord          = tangent::getTexCoord;
                interface.m_setTSpaceBasic       = tangent::setTSpaceBasic;

                SMikkTSpaceContext context {};
                context.m_pInterface = &interface;
                context.m_pUserData  = &nblContext;

                if (genTangSpaceDefault(&context))
                {
                    spdlog::debug("Generated tangents for {}", geometryName);
                }
                else
                {
                    spdlog::warn("Failed to generate tangents for {}", geometryName);
                }
            }

            const auto geometry = mSceneGeometry->addGeometry<Geometry>(GeometryCreateInfo {
                .vertices = vertices,
                .indices  = indices,
                .name     = geometryName,
            });

            SplashWindow::get().setMessage(std::format("Loaded geometry: {}", geometryName), mSceneName);

            // Material
            MaterialData material = {};

            if (prim.materialIndex.has_value())
            {
                const auto& mat = asset.materials[*prim.materialIndex];
                const auto& pbr = mat.pbrData;

                material.solidColor       = glm::make_vec4(&pbr.baseColorFactor[0]);
                material.pMetallicFactor  = pbr.metallicFactor;
                material.pRoughnessFactor = pbr.roughnessFactor;

                if (pbr.baseColorTexture.has_value())
                {
                    // texUV = pbr.baseColorTexture->texCoordIndex;
                    const auto gltfTexIdx = static_cast<int32_t>(pbr.baseColorTexture->textureIndex);
                    if (mTextureMap.contains(gltfTexIdx))
                    {
                        material.hTexture = mTextureMap[gltfTexIdx];
                    }
                }
                if (mat.normalTexture.has_value())
                {
                    // normalUV = mat.normalTexture->texCoordIndex;
                    const auto gltfTexIdx = static_cast<int32_t>(mat.normalTexture->textureIndex);
                    if (mTextureMap.contains(gltfTexIdx))
                    {
                        material.hNormalMap = mTextureMap[gltfTexIdx];
                    }
                }
                if (pbr.metallicRoughnessTexture.has_value())
                {
                    // mrUV = pbr.metallicRoughnessTexture->texCoordIndex;
                    const auto gltfTexIdx = static_cast<int32_t>(pbr.metallicRoughnessTexture->textureIndex);
                    if (mTextureMap.contains(gltfTexIdx))
                    {
                        material.hMetallicRoughnessMap = mTextureMap[gltfTexIdx];
                    }
                }
            }

            mMeshMap[meshIndex].push_back({
                .geometryIndex = geometry,
                .hMaterial     = mMaterialPool->acquire(material),
                .aabb          = aabb,
            });
        }
    }
}

void GLTFLoader::s4_walkNodes(fastgltf::Asset& asset) noexcept
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
    const glm::mat4 model = getWorldTransform(node.transform, parentModel);

    bool isStringLight = false;
    if (node.meshIndex.has_value())
    {
        std::string meshName(asset.meshes[*node.meshIndex].name);
        std::string nodeName(node.name);
        bool isEmissive = nodeName.contains("Light") || meshName.contains("Light");
        isStringLight = nodeName.contains("StringLight");

        std::unordered_set<size_t> spawnedLightNodes;
        // Spawn lights at emissive mesh locations
        if (isEmissive && !spawnedLightNodes.contains(*node.meshIndex))
        {
            spawnedLightNodes.insert(nodeIndex);
            auto& mesh = asset.meshes[*node.meshIndex];
            if (!mesh.primitives.empty())
            {
                auto& prim = mesh.primitives[0];

                // Position from bounding box center
                glm::vec3 position = glm::vec3(model[3]);
                const auto* posAttr = prim.findAttribute("POSITION");
                if (posAttr != prim.attributes.end())
                {
                    auto& acc = asset.accessors[posAttr->accessorIndex];
                    glm::vec3 sum(0.0f);
                    size_t count = 0;
                    fastgltf::iterateAccessor<glm::vec3>(asset, acc, [&](glm::vec3 pos) {
                        sum += pos;
                        count++;
                    });
                    if (count > 0)
                    {
                        glm::vec3 center = sum / static_cast<float>(count);
                        position = glm::vec3(model * glm::vec4(center, 1.0f));
                    }
                }

                // Color from emissive, fallback to base color
                const auto c = Random::getColor();

                // mLightSystem->addLight({
                //     .position    = position,
                //     .color       = { c[0], c[1], c[2] },
                //     .intensity   = 25.0f,
                //     .enabled     = true,
                //     .castsShadow = false,
                //     .type        = LightType::Point,
                // });
            }
        }

        // TODO: Lights
        //if (node.lightIndex.has_value())
        //{
        //    auto& light = asset.lights[*node.lightIndex];
        //    if (light.type == fastgltf::LightType::Point)
        //    {
        //        glm::vec3 position = glm::vec3(world[3]);
        //        glm::vec3 color(light.color[0], light.color[1], light.color[2]);
        //        // mLightSystem->addLight({
        //        //     .position    = position,
        //        //     .color       = color,
        //        //     .intensity   = 25.0f, //light.intensity,
        //        //     .enabled     = true,
        //        //     .castsShadow = true,
        //        //     .type        = LightType::Point,
        //        // });
        //    }
        //}

        if (const auto it = mMeshMap.find(static_cast<int32_t>(*node.meshIndex)); it != mMeshMap.end())
        {
            for (const auto& prim : it->second)
            {
                if (isStringLight)
                {
                    mMaterialPool->modify(prim.hMaterial, [](MaterialData& data) -> void {
                        data.pIsEmissive = true;
                    });
                }
                const auto generatedName = node.name.empty()
                    ? fmt::format("gltf_node_{}", nodeIndex)
                    : std::string(node.name);
                auto transform = Transform().setModel(model);
                mScene->addObject<Object>(prim.geometryIndex, transform, prim.hMaterial, generatedName);
            }
        }
    }

    for (const auto childIndex : node.children)
    {
        processNode(asset, childIndex, model);
    }
}

std::string GLTFLoader::getTextureName(const fastgltf::Image& image, const uint64_t index) const noexcept
{
    return image.name.empty()
           ? fmt::format("tex_{}_{}", mFileName.string(), index)
           : fmt::format("tex_{}", image.name);
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
