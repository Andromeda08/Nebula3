#include "SceneManager.hpp"

#include "IO/SceneLoader.hpp"

namespace nbl
{
    Scene* SceneManager::loadScene(const std::filesystem::path& path)
    {
        std::scoped_lock loadLock(mLoadMutex);

        auto result = SceneLoader(mGeometrySystem).loadScene(path);
        if (!result)
        {
            spdlog::error("Failed to load scene from: {}", path.string());
            return nullptr;
        }

        std::scoped_lock lock(mScenesMutex);

        mScenes.push_back(std::move(result));
        if (!mActiveScene.has_value())
        {
            mActiveScene = static_cast<uint32_t>(mScenes.size() - 1);
        }

        return mScenes.back().get();
    }

    //std::future<Scene*> SceneManager::loadSceneAsync(const std::filesystem::path& path)
    //{
    //    std::packaged_task task([this, path] -> Scene* {
    //        return loadScene(path);
    //    });
    //
    //    auto future = task.get_future();
    //    std::thread(std::move(task)).detach();
    //    return future;
    //}

    void SceneManager::setActiveScene(const uint32_t i) noexcept
    {
        if (i >= mScenes.size())
        {
            mActiveScene = mScenes.empty() ? std::nullopt : std::make_optional(0u);
            return;
        }
        mActiveScene = i;
    }

    Scene* SceneManager::getActiveScene() const noexcept
    {
        if (mActiveScene.has_value())
        {
            return mScenes[mActiveScene.value()].get();
        }
        return nullptr;
    }
}
