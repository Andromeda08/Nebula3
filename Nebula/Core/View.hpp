#pragma once

#include <string>
#include <typeindex>
#include <unordered_map>
#include <SDL3/SDL.h>

#include "Core/Types.hpp"
#include "Level/TextureManager.hpp"
#include "Input/Gamepad.hpp"
#include "UserInterface/UserInterface.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    #define nbl_ViewCtorParams GamepadManager* pGamepad, const SPtr<RHI::VulkanRHI>& rhi, \
        TextureManager* pTextureManager, UserInterface* pUserInterface

    #define nbl_ViewBaseCtor View(pGamepad, rhi, pTextureManager, pUserInterface)

    class View
    {
    public:
        View(
            GamepadManager*             pGamepad,
            const SPtr<RHI::VulkanRHI>& rhi,
            TextureManager*             pTextureManager,
            UserInterface*              pUserInterface)
        : mGamepad(pGamepad)
        , mRHI(rhi)
        , mTextureManager(pTextureManager)
        , mUserInterface(pUserInterface)
        {
        }

        virtual ~View() = default;

        virtual void onEvent(const SDL_Event& event) = 0;

        virtual void onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) = 0;

        virtual void onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) = 0;

        virtual void onDrawUI() {}

        [[nodiscard]] const std::string& getName() const noexcept
        {
            return mName;
        }

    protected:
        GamepadManager*      mGamepad;
        SPtr<RHI::VulkanRHI> mRHI;
        TextureManager*      mTextureManager;
        UserInterface*       mUserInterface;

        std::string          mName;
    };

    class ViewSelectUI : public IComponent
    {
    public:
        explicit ViewSelectUI(std::unordered_map<std::type_index, UPtr<View>>& views, View** ppActiveView)
        : IComponent()
        , mViews(views)
        , mActiveViewRef(ppActiveView)
        {
        }

        void draw() override;

    private:
        std::unordered_map<std::type_index, UPtr<View>>& mViews;
        View**                                           mActiveViewRef = nullptr;
    };
}
