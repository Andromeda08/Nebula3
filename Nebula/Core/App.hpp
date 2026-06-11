#pragma once

#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "Types.hpp"
#include "View.hpp"
#include "Input/Gamepad.hpp"
#include "Math/DeltaTime.hpp"
#include "UserInterface/UserInterface.hpp"
#include "VulkanRHI/VulkanRHI.hpp"
#include "Window/SDLWindow.hpp"

class App
{
public:
    nbl_DISABLE_COPY(App);

    App();
    static UPtr<App> create() noexcept;

    template <class T, class... Args>
    requires std::is_base_of_v<nbl::View, T>
    T* addView(Args&&... args)
    {
        const auto typeIndex = std::type_index(typeid(T));
        if (!mViews.contains(typeIndex))
        {
            mViews.insert_or_assign(typeIndex, makeUnique<T>(
                mGamepadManager.get(), mVulkanRHI, mTextureManager.get(), mUserInterface.get(),
                std::forward<Args>(args)...));
            if (!mActiveView)
            {
                mActiveView = mViews[typeIndex].get();
                spdlog::info("Active View: {}", mActiveView->getName());
            }
        }
        else
        {
            spdlog::warn("A View of type {} already exists.", typeid(T).name());
        }
        return static_cast<T*>(mViews[typeIndex].get());
    }

    template <class T>
    requires std::is_base_of_v<nbl::View, T>
    [[nodiscard]] T* getView()
    {
        const auto it = mViews.find(std::type_index(typeid(T)));
        return (it != std::end(mViews))
            ? static_cast<T*>(it->second.get())
            : nullptr;
    }

    void run();

    ~App();

private:
    bool                         mRunning = false;
    float                        mCPUFramerate = 0.0f;

    DeltaTime                    mDeltaTime;
    SPtr<SDLWindow>              mWindow;
    UPtr<GamepadManager>         mGamepadManager;

    SPtr<RHI::VulkanRHI>         mVulkanRHI;
    UPtr<TextureManager>         mTextureManager;

    UPtr<UserInterface>          mUserInterface;

    std::unordered_map<std::type_index, UPtr<nbl::View>> mViews;
    nbl::View*                                           mActiveView = nullptr;
};

extern App* gApplication;
