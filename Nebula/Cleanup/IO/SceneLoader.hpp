#pragma once

#include <filesystem>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Cleanup/Scene.hpp"
#include "Cleanup/Geometry/GeometrySystem.hpp"
#include "Core/Types.hpp"

namespace nbl
{
    class SceneLoader
    {
    public:
        explicit SceneLoader(GeometrySystem* pGeometrySystem)
        : mGeometrySystem(pGeometrySystem)
        {
            mLogger = spdlog::stdout_color_mt("SceneLoader");
            mLogger->set_pattern("[%^%l%$] %v");
        }

        [[nodiscard]] UPtr<Scene> loadScene(const std::filesystem::path& path);

    private:
        void loadGeometries(const fastgltf::Asset& asset);

        void createObjects(fastgltf::Asset& asset, Scene* pScene);

        void processNode(fastgltf::Asset& asset, Scene* pScene, size_t nodeIndex, const glm::mat4& parentModel) noexcept;

        template <class vec>
        std::vector<vec> loadAccessorData(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, const std::string_view name)
        {
            const auto* attribute = primitive.findAttribute(name);
            if (attribute == primitive.attributes.end())
            {
                return {};
            }

            return loadAccessorData<vec>(asset, asset.accessors[attribute->accessorIndex]);
        }

        template <class vec>
        std::vector<vec> loadAccessorData(const fastgltf::Asset& asset, const fastgltf::Accessor& accessor)
        {
            std::vector<vec> result(accessor.count);
            fastgltf::iterateAccessorWithIndex<vec>(asset, accessor, [&](vec v, const auto i) -> void
            {
                result[i] = v;
            });
            return result;
        }

        SPtr<spdlog::logger>    mLogger;
        GeometrySystem*         mGeometrySystem;

        // mGeometryMap[meshIndex][primitiveIndex] = mGeometrySystem index
        std::vector<std::vector<GeometryIndex>> mGeometryMap;
    };
}
