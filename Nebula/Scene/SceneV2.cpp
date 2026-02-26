#include "SceneV2.hpp"

#include <execution>
#include <unordered_map>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <stb_image.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Math/DeltaTime.hpp"
#include "Window/SplashWindow.hpp"

struct PrimitiveInfo
{
    SPtr<Geometry>  geometry;
    int32_t         textureIndex;
    glm::vec4       baseColor;
};

struct TextureInfo
{
    int32_t     width;
    int32_t     height;
    stbi_uc*    pixels;
    int32_t     slot;
    std::string label;
};

namespace detail
{
    [[nodiscard]] static std::string getTextureName(const fastgltf::Image& image, const std::string& fileName, const uint64_t idx) noexcept
    {
        return image.name.empty()
            ? std::format("gltf_{}_{}", fileName, idx)
            : std::string(image.name);
    }
}

SceneV2::SceneV2(const SPtr<RHI::VulkanRHI>& rhi)
: mRHI(rhi)
{
    mGeometry = makeUnique<SceneGeometry>(mRHI);
    mInstancePool = makeUnique<InstancePool>(mRHI, 65536);
    mTextureManager = TextureManager::create({ mRHI });

    if (mRHI->getRaytracingSupport())
    {
        mTLASManager = TLASManager::create({ mRHI, mInstancePool.get() });
    }

    mLightSystem = makeUnique<LightSystem>(mRHI);

    for (auto&& [i, buffer] : nbl::enumerate(mCameraUniformBuffers))
    {
        buffer = mRHI->createBuffer({
            .size = sizeof(CameraData),
            .type = RHI::BufferType::Uniform,
            .label = std::format("Scene_Uniform_Camera_{}", i),
        });
    }

    {
        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        mCamera = makeUnique<FlyingCamera>(glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));

        mLightSystem->addLight({
            .position = { -17.0f, 15.0f, -1.0f },
            .color = { 241.0f / 255.0f, 241.0f / 255.0f, 204.0f / 255.0f },
            .intensity = 750.0f,
            .enabled = true,
            .castsShadow = true,
            .type = LightType::Point,
            .name = "Highlight"
        });
        mLightSystem->addLight({
            .position = { -10, 250, 10 },
            .color = { 232.0f / 255.0f, 243.0f / 255.0f, 240.0f / 255.0f },
            .intensity = 75000.0f,
            .enabled = true,
            .castsShadow = true,
            .type = LightType::Point,
            .name = "Sky"
        });

        fast_parseGLTFScene("bistro.glb");
    }

    // initScene();

    std::vector bindings = {
        vk::DescriptorSetLayoutBinding {
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute
        },
        vk::DescriptorSetLayoutBinding {
            1, vk::DescriptorType::eStorageBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute
        },
    };
    if (mRHI->getRaytracingSupport())
    {
        using enum vk::ShaderStageFlagBits;
        bindings.push_back({
            2, vk::DescriptorType::eAccelerationStructureKHR, 1,
            eVertex | eFragment | eCompute | eRaygenKHR | eAnyHitKHR | eClosestHitKHR | eMissKHR | eIntersectionKHR | eCallableKHR
        });
    }

    /* TODO: Bindless */ {
        mSceneDescriptor = mRHI->createDescriptor({
            .bindings = bindings,
            .setCount = 2,
            .debugName = "Scene_Descriptor",
        });

        for (auto i = 0; i < mSceneDescriptor->getSetCount(); i++)
        {
            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeUniformBuffer(0, mCameraUniformBuffers[i])
                .writeStorageBuffer(1, mLightSystem->getDataBuffer())
                .writeAccelerationStructure(2, mTLASManager->getTLAS());
            mSceneDescriptor->write(i, descriptorWrite);
        }
    }

    const auto extent = mRHI->getSwapchain()->getProperties().extent;
    mTestPass = Indirect_GBufferPass::create({
        .resolution ={ extent.width, extent.height },
        .pScene = this,
        .rhi = mRHI,
    });

    mSSAO = SSAOPass::create({
        .useBlur    = true,
        .resolution = { extent.width, extent.height },
        .input      = { mTestPass->getPosition(), mTestPass->getNormal(), mSceneDescriptor },
        .rhi        = mRHI,
    });

    mRTAO = RTAOPass::create({
        .resolution = { extent.width, extent.height },
        .input      = { mTestPass->getPosition(), mTestPass->getNormal(), mSceneDescriptor },
        .rhi        = mRHI,
    });

    mLightingPass = LightingPass::create({
        .resolution = { extent.width, extent.height },
        .input      = { mTestPass->getPosition(), mTestPass->getNormal(), mTestPass->getAlbedo(), mSceneDescriptor, mSSAO->getResult() },
        .rhi        = mRHI,
    });

    mFXAA = FXAAPass::create({
        .resolution = { extent.width, extent.height },
        .input      = { mLightingPass->getResult() },
        .rhi        = mRHI,
    });
}

void SceneV2::fast_parseGLTFScene(const std::string& fileName) noexcept
{
    mName = std::filesystem::path(fileName).filename().stem().string();

    DeltaTime dt = {};
    dt.initialize();

    // Parse GLB
    DeltaTime iodt = {};
    iodt.initialize();
    fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);

    auto data = fastgltf::GltfDataBuffer::FromPath(fileName);
    exitOnAssert(data.error() == fastgltf::Error::None, "Failed to load file: {}", fileName);

    constexpr auto options =fastgltf::Options::LoadExternalBuffers | fastgltf::Options::GenerateMeshIndices;

    auto assetResult = parser.loadGltf(data.get(), std::filesystem::path("."), options);
    exitOnAssert(assetResult.error() == fastgltf::Error::None, "Failed to parse GLTF ({}): {}", fastgltf::getErrorMessage(assetResult.error()), fileName);
    const auto ioTime = iodt.getDeltaTime();
    spdlog::debug("gltf IO time: {}s", ioTime);

    auto& asset = assetResult.get();

    // Load Textures
    // auto textureBatch = mTextureManager->createBatchUpload();

    std::unordered_map<int32_t, int32_t> textureMap;
    int32_t slotCounter = 2;

    int32_t c_fallback = 0;
    int32_t c_uri = 0;
    int32_t c_array = 0;
    int32_t c_vector = 0;
    int32_t c_buffer_fallback = 0;
    int32_t c_buffer_array = 0;
    int32_t c_buffer_vector = 0;
    for (size_t texIdx = 0; texIdx < asset.textures.size(); texIdx++)
    {
        auto& texture = asset.textures[texIdx];
        if (!texture.imageIndex.has_value())
        {
            continue;
        }

        auto& image = asset.images[*texture.imageIndex];
        std::string name = image.name.empty()
            ? std::format("gltf_{}_{}", fileName, texIdx)
            : std::string(image.name);

        int width = 0, height = 0, channels = 0;
        unsigned char* pixels = nullptr;

        // image.data is a std::variant (DataSource) — visit to extract bytes
        std::visit(fastgltf::visitor {
            [&](auto&)
            {
                c_fallback++;
            }, // fallback for unsupported sources
            [&](fastgltf::sources::URI& filePath) {
                const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
                pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
                c_uri++;
            },
            [&](fastgltf::sources::Array& array) {
                pixels = stbi_load_from_memory(
                    reinterpret_cast<const stbi_uc*>(array.bytes.data()),
                    static_cast<int>(array.bytes.size()),
                    &width, &height, &channels, 4);
                c_array++;
            },
            [&](fastgltf::sources::Vector& vector) {
                pixels = stbi_load_from_memory(
                    reinterpret_cast<const stbi_uc*>(vector.bytes.data()),
                    static_cast<int>(vector.bytes.size()),
                    &width, &height, &channels, 4);
                c_vector++;
            },
            [&](fastgltf::sources::BufferView& view) {
                auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                auto& buffer = asset.buffers[bufferView.bufferIndex];
                std::visit(fastgltf::visitor {
                    [&](auto&) { c_buffer_fallback++; },
                    [&](fastgltf::sources::Array& array) {
                        pixels = stbi_load_from_memory(
                            reinterpret_cast<const stbi_uc*>(array.bytes.data() + bufferView.byteOffset),
                            static_cast<int>(bufferView.byteLength),
                            &width, &height, &channels, 4);
                        c_buffer_array++;
                    },
                    [&](fastgltf::sources::Vector& vector) {
                        pixels = stbi_load_from_memory(
                            reinterpret_cast<const stbi_uc*>(vector.bytes.data() + bufferView.byteOffset),
                            static_cast<int>(bufferView.byteLength),
                            &width, &height, &channels, 4);
                        c_buffer_vector++;
                    },
                }, buffer.data);
            },
        }, image.data);

        if (pixels)
        {
            uint32_t s = mTextureManager->loadTextureFromMemory(name, pixels, width, height);
            //textureBatch.addTexture(name, pixels, width, height, slotCounter);
            stbi_image_free(pixels);
            // spdlog::info("Loaded texture [slot={}]: {}", slotCounter, name);
            SplashWindow::get().setMessage(std::format("Loaded texture [slot={}]: {}", s, name), mName);
            textureMap[static_cast<int32_t>(texIdx)] = s;
            slotCounter++;
        }
        else
        {
            spdlog::warn("Failed to decode texture: {}", name);
        }
    }

    spdlog::info("Image source stats: {}, {}, {}, {}, {}, {}, {}", c_fallback, c_uri, c_array, c_vector, c_buffer_fallback, c_buffer_array, c_buffer_vector);

    // mTextureManager->loadTextureBatch(textureBatch);

    // Load geometries
    std::unordered_map<int32_t, std::vector<PrimitiveInfo>> meshPrimitives;
    for (size_t meshIdx = 0; meshIdx < asset.meshes.size(); meshIdx++)
    {
        auto& mesh = asset.meshes[meshIdx];

        for (size_t primIdx = 0; primIdx < mesh.primitives.size(); primIdx++)
        {
            auto& prim = mesh.primitives[primIdx];
            if (prim.type != fastgltf::PrimitiveType::Triangles) continue;

            // Position
            const auto* posAccessorIdx = prim.findAttribute("POSITION");
            if (posAccessorIdx == prim.attributes.end()) continue;

            auto& posAccessor = asset.accessors[posAccessorIdx->accessorIndex];
            const size_t vertexCount = posAccessor.count;

            std::vector<Vertex> vertices(vertexCount);

            // Read positions
            fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, posAccessor,
                [&](glm::vec3 pos, size_t idx) {
                    vertices[idx].position = pos;
                });

            // Read normals
            if (const auto* attr = prim.findAttribute("NORMAL"); attr != prim.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, asset.accessors[attr->accessorIndex],
                    [&](glm::vec3 n, size_t idx) {
                        vertices[idx].normal = n;
                    });
            }

            // Read UVs
            if (const auto* attr = prim.findAttribute("TEXCOORD_0"); attr != prim.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec2>(asset, asset.accessors[attr->accessorIndex],
                    [&](glm::vec2 uv, size_t idx) {
                        vertices[idx].uv = { uv.x, 1.0f - uv.y };
                    });
            }

            // Read indices
            std::vector<uint32_t> indices;
            if (prim.indicesAccessor.has_value())
            {
                auto& indexAccessor = asset.accessors[*prim.indicesAccessor];
                indices.resize(indexAccessor.count);
                fastgltf::iterateAccessorWithIndex<uint32_t>(asset, indexAccessor,
                    [&](uint32_t index, size_t idx) {
                        indices[idx] = index;
                    });
            }

            std::string geometryName = std::format("gltf_{}_mesh{}_prim{}", fileName, meshIdx, primIdx);
            auto geometry = mGeometry->addGeometry<Geometry>(GeometryCreateInfo {
                .vertices = vertices,
                .indices  = indices,
                .name     = geometryName,
            });
            // spdlog::info();
            SplashWindow::get().setMessage(std::format("Loaded geometry: {}", geometryName), mName);

            // Material
            int32_t   texSlot = -1;
            glm::vec4 baseColor(0.8f, 0.8f, 0.8f, 1.0f);

            if (prim.materialIndex.has_value())
            {
                auto& mat = asset.materials[*prim.materialIndex];
                auto& pbr = mat.pbrData;

                baseColor = glm::vec4(
                    pbr.baseColorFactor[0], pbr.baseColorFactor[1],
                    pbr.baseColorFactor[2], pbr.baseColorFactor[3]);

                if (pbr.baseColorTexture.has_value())
                {
                    auto gltfTexIdx = static_cast<int32_t>(pbr.baseColorTexture->textureIndex);
                    if (textureMap.contains(gltfTexIdx))
                    {
                        texSlot = textureMap[gltfTexIdx];
                    }
                }
            }

            meshPrimitives[static_cast<int32_t>(meshIdx)].push_back({
                .geometry     = geometry,
                .textureIndex = texSlot,
                .baseColor    = baseColor,
            });
        }
    }

    mGeometry->onUpdate();

    std::function<void(size_t, glm::mat4)> processNode = [&](size_t nodeIdx, glm::mat4 parentMatrix)
    {
        auto& node = asset.nodes[nodeIdx];

        // Build local matrix from transform variant (TRS or Matrix)
        glm::mat4 local(1.0f);
        std::visit(fastgltf::visitor {
            [&](fastgltf::math::fmat4x4& matrix) {
                std::memcpy(&local, matrix.data(), sizeof(glm::mat4));
            },
            [&](fastgltf::TRS& trs) {
                glm::vec3 t(trs.translation[0], trs.translation[1], trs.translation[2]);
                glm::quat r(trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]);
                glm::vec3 s(trs.scale[0], trs.scale[1], trs.scale[2]);
                local = glm::translate(glm::mat4(1.0f), t)
                      * glm::mat4_cast(r)
                      * glm::scale(glm::mat4(1.0f), s);
            },
        }, node.transform);

        glm::mat4 world = parentMatrix * local;

        if (node.meshIndex.has_value())
        {
            std::string meshName(asset.meshes[*node.meshIndex].name);
            std::string nodeName(node.name);

            bool isEmissive = nodeName.contains("Light") || meshName.contains("Light");

            std::unordered_set<size_t> spawnedLightNodes;
            // Spawn lights at emissive mesh locations
            if (isEmissive && !spawnedLightNodes.contains(*node.meshIndex))
            {
                spawnedLightNodes.insert(nodeIdx);
                auto& mesh = asset.meshes[*node.meshIndex];
                if (!mesh.primitives.empty())
                {
                    auto& prim = mesh.primitives[0];

                    // Position from bounding box center
                    glm::vec3 position = glm::vec3(world[3]);
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
                            position = glm::vec3(world * glm::vec4(center, 1.0f));
                        }
                    }

                    // Color from emissive, fallback to base color
                    const auto c = Random::getColor();

                    mLightSystem->addLight({
                        .position    = position,
                        .color       = { c[0], c[1], c[2] },
                        .intensity   = 25.0f,
                        .enabled     = true,
                        .castsShadow = false,
                        .type        = LightType::Point,
                    });
                    // spdlog::info("Added light");
                }
            }

            auto it = meshPrimitives.find(static_cast<int32_t>(*node.meshIndex));
            if (it != meshPrimitives.end())
            {
                for (const auto& prim : it->second)
                {
                    auto t = Transform().setModel(world);
                    addObject<Object>(prim.geometry, prim.textureIndex, t);
                    mObjects.back()->solidColor = prim.baseColor;
                    mObjects.back()->name = node.name.empty()
                        ? std::format("gltf_node_{}", nodeIdx)
                        : std::string(node.name);
                }
            }
        }

        if (node.lightIndex.has_value())
        {
            auto& light = asset.lights[*node.lightIndex];
            if (light.type == fastgltf::LightType::Point)
            {
                glm::vec3 position = glm::vec3(world[3]);
                glm::vec3 color(light.color[0], light.color[1], light.color[2]);

                mLightSystem->addLight({
                    .position    = position,
                    .color       = color,
                    .intensity   = light.intensity,
                    .enabled     = true,
                    .castsShadow = true,
                    .type        = LightType::Point,
                });
            }
        }

        for (auto child : node.children)
        {
            processNode(child, world);
        }
    };

    size_t sceneIndex = asset.defaultScene.has_value() ? *asset.defaultScene : 0;
    auto& scene = asset.scenes[sceneIndex];
    for (auto nodeIdx : scene.nodeIndices)
    {

        processNode(nodeIdx, glm::mat4(1.0f));
    }
    const auto time = dt.getDeltaTime();
    spdlog::info("Loaded {} in {}s (IO time: {}s)", fileName, time, ioTime);
}
