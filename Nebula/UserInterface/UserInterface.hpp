#pragma once

#include "IComponent.hpp"
#include "ImGuiRenderPass.hpp"
#include "Core/Macro.hpp"
#include "Window/SDLWindow.hpp"

struct UserInterfaceCreateInfo
{
    std::string          fontFile;
    SPtr<SDLWindow>      window;
    SPtr<RHI::VulkanRHI> rhi;
};

class UserInterface
{
public:
    nbl_DISABLE_COPY(UserInterface);
    nbl_CTOR(UserInterface);

    template <class T, class... Args>
    void addComponent(Args&&... args)
    {
        mComponents.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void update() const;

    void processEvents(const SDL_Event& event) noexcept;

    void draw(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const;

    static bool wantCaptureInput() noexcept;

    static bool wantCaptureMouse() noexcept;

    static bool wantCaptureKeyboard() noexcept;

private:
    UPtr<ImGuiRenderPass>         mRenderer;
    std::vector<UPtr<IComponent>> mComponents;
};
