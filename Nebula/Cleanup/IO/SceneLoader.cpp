#include "SceneLoader.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include "Cleanup/Timer.hpp"
#include "Cleanup/Geometry/Geometry.hpp"
#include "Cleanup/Geometry/TangentGeneration.hpp"
#include "Core/Ranges.hpp"

namespace nbl
{
    namespace GLTF
    {
        static constexpr auto AttribPosition  = "POSITION";
        static constexpr auto AttribNormal    = "NORMAL";
        static constexpr auto AttribTangent   = "TANGENT";
        static constexpr auto AttribTexCoord0 = "TEXCOORD_0";
        static constexpr auto AttribTexCoord1 = "TEXCOORD_0";
    }

    UPtr<Scene> SceneLoader::loadScene(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            mLogger->error("Invalid path: {}", path.string());
        }

        if (const auto ext = path.extension(); ext != ".gltf" || ext != ".glb")
        {
            // mLogger->error("Only GLTF (.gltf, .glb) files are supported!");
            // return nullptr;
        }

        mLogger->info("[SceneLoader] Loading scene: {}", path.string());

        const auto name = path.filename().stem().string();

        // Parse and Load GLTF File
        // ===================================
        auto ioTimer = Timer().start();
        #pragma region

        fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);
        auto gltfDataBuffer = fastgltf::GltfDataBuffer::FromPath(path);
        if (gltfDataBuffer.error() != fastgltf::Error::None)
        {
            exitWithError("Failed to load GLTF file ({}): {}", fastgltf::getErrorMessage(gltfDataBuffer.error()), path.string());
        }

        using opt = fastgltf::Options;
        constexpr auto options = opt::LoadExternalBuffers | opt::GenerateMeshIndices | opt::DecomposeNodeMatrices;

        auto assetResult = parser.loadGltf(gltfDataBuffer.get(), std::filesystem::path("."), options);
        if (assetResult.error() != fastgltf::Error::None)
        {
            exitWithError("Failed to parse GLTF ({}): {}", fastgltf::getErrorMessage(assetResult.error()), path.string());
        }

        #pragma endregion
        const auto ioTime = ioTimer.stop();

        mLogger->debug("[SceneLoader] Scene loading IO Time: {}s ({})", ioTime, name);

        auto& asset = assetResult.get();

        loadGeometries(asset);
        mGeometrySystem->commit();

        auto scene = makeUnique<Scene>(name);

        createObjects(asset, scene.get());

        return std::move(scene);
    }

    void SceneLoader::loadGeometries(const fastgltf::Asset& asset)
    {
        mGeometryMap.resize(asset.meshes.size());
        for (const auto& [meshIndex, mesh] : nbl::enumerate(asset.meshes))
        {
            mGeometryMap[meshIndex].resize(mesh.primitives.size());
            for (const auto& [primIndex, primitive] : nbl::enumerate(mesh.primitives))
            {
                std::string geometryName = fmt::format("geo_mesh{}_prim{}", meshIndex, primIndex);

                if (primitive.type != fastgltf::PrimitiveType::Triangles)
                {
                    continue;
                }

                // Check if the primitive has any position data
                const auto* posAttr = primitive.findAttribute(GLTF::AttribPosition);
                if (posAttr == primitive.attributes.end())
                {
                    exitWithError("No position data for primitive.");
                }

                // Check if the primitive has any index data
                if (!primitive.indicesAccessor.has_value())
                {
                    exitWithError("No index data for primitive.");
                }

                auto& posAccessor = asset.accessors[posAttr->accessorIndex];
                const size_t vertexCount = posAccessor.count;

                // Load data
                // ============================
                std::vector<glm::vec3> positions = loadAccessorData<glm::vec3>(asset, posAccessor);
                std::vector<glm::vec3> normals   = loadAccessorData<glm::vec3>(asset, primitive, GLTF::AttribNormal);
                std::vector<glm::vec4> tangents  = loadAccessorData<glm::vec4>(asset, primitive, GLTF::AttribTangent);
                std::vector<glm::vec2> texcoords = loadAccessorData<glm::vec2>(asset, primitive, GLTF::AttribTexCoord0);

                // Generate Tangents later with mikktspace if empty
                const auto shouldGenerateTangents = tangents.empty();

                std::vector<uint32_t> indices = loadAccessorData<uint32_t>(asset, asset.accessors[*primitive.indicesAccessor]);
                const size_t indexCount = indices.size();

                if (shouldGenerateTangents)
                {
                    Tangent::generateTangents(vertexCount, {
                        .positions = &positions,
                        .normals   = &normals,
                        .texcoords = &texcoords,
                        .indices   = &indices,
                        .tangents  = &tangents,
                    });
                }

                std::vector<Vertex> vertices = positions
                    | std::views::transform([](const glm::vec3& p) -> Vertex { return { .position = p }; })
                    | std::ranges::to<std::vector>();

                std::vector<VertexAttributes> attributes(vertexCount);
                for (auto i = 0; i < vertexCount; i++)
                {
                    attributes[i] = {
                        .normal   = glm::vec4(normals[i], 0.0f) ,
                        .tangent  = tangents[i],
                        .texcoord = texcoords[i],
                    };
                }

                GeometryCreateInfo createInfo = {
                    .vertices   = std::move(vertices),
                    .attributes = std::move(attributes),
                    .indices    = std::move(indices),
                    .name       = std::move(geometryName),
                };

                const auto geometryIndex = mGeometrySystem->addGeometry<Geometry>(createInfo);
                mGeometryMap[meshIndex][primIndex] = geometryIndex;
            }
        }
    }

    void SceneLoader::createObjects(fastgltf::Asset& asset, Scene* pScene)
    {
        const size_t sceneIndex = asset.defaultScene.has_value() ? *asset.defaultScene : 0;
        const auto&  scene      = asset.scenes[sceneIndex];

        for (const auto nodeIndex : scene.nodeIndices)
        {
            processNode(asset, pScene, nodeIndex, glm::mat4(1.0f));
        }
    }

    void SceneLoader::processNode(fastgltf::Asset& asset, Scene* pScene, const size_t nodeIndex, const glm::mat4& parentModel) noexcept
    {
        auto& node = asset.nodes[nodeIndex];

        // DecomposeNodeMatrices -> TRS Variant
        const auto& trs = std::get<fastgltf::TRS>(node.transform);

        auto transform = Transform()
            .setMode(TransformType::Quaternion)
            .setTranslate(glm::make_vec3(trs.translation.data()))
            .setScale(glm::make_vec3(trs.scale.data()))
            .setRotationQuat(glm::make_quat(trs.rotation.data()));

        const auto worldTransform = parentModel * transform.getModel();

        if (node.meshIndex.has_value())
        {
            const size_t meshIndex = *node.meshIndex;
            const auto&  mesh      = asset.meshes[meshIndex];

            for (size_t primIndex = 0; primIndex < mesh.primitives.size(); primIndex++)
            {
                const GeometryIndex geometryIndex = mGeometryMap[meshIndex][primIndex];
                if (geometryIndex == -1)
                {
                    continue;
                }

                auto objectParams = ObjectParams {
                    .name          = fmt::format("{}_obj", mGeometrySystem->getGeometry(geometryIndex)->getName()),
                    .pGeometry     = mGeometrySystem->getGeometry(geometryIndex),
                    .geometryInfo  = mGeometrySystem->getGeometryMeta(geometryIndex),
                    .geometryIndex = geometryIndex,
                    .materialIndex = 0,
                    .instanceIndex = 0,
                    .transform     = Transform(worldTransform),
                };

                pScene->addObject<Object>(objectParams);
            }
        }

        for (const size_t childIndex : node.children)
        {
            processNode(asset, pScene, childIndex, worldTransform);
        }
    }
}
