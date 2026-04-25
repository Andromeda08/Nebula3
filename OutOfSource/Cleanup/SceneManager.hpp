#pragma once

#include <future>
#include <mutex>
#include <optional>
#include <vector>

#include "Scene.hpp"
#include "Core/Types.hpp"
#include "Geometry/GeometrySystem.hpp"

namespace nbl
{
    class SceneManager
    {
    public:
        explicit SceneManager(GeometrySystem* pGeometrySystem)
        : mGeometrySystem(pGeometrySystem)
        , mActiveScene(std::nullopt)
        {
            if (!mGeometrySystem)
            {
                exitWithError("GeometrySystem is null");
            }
        }

        Scene* loadScene(const std::filesystem::path& path);

        // std::future<Scene*> loadSceneAsync(const std::filesystem::path& path);

        void setActiveScene(uint32_t i) noexcept;

        [[nodiscard]] Scene* getActiveScene() const noexcept;

    private:
        GeometrySystem*             mGeometrySystem;

        mutable std::mutex          mScenesMutex;
        std::mutex                  mLoadMutex;

        std::vector<UPtr<Scene>>    mScenes;
        std::optional<uint32_t>     mActiveScene;
    };
}
