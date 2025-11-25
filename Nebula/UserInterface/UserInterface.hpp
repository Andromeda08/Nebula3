#pragma once

#include "ImGuiRenderer.hpp"
#include "Core/Macro.hpp"

struct UserInterfaceCreateInfo
{
    std::string          fontFile;
    SPtr<Window>         window;
    SPtr<RHI::VulkanRHI> rhi;
};

class UserInterface
{
public:
    nbl_DISABLE_COPY(UserInterface);
    nbl_CTOR(UserInterface);

    void update();

    void draw(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const;

    bool wantCaptureMouse() const noexcept;

    bool wantCaptureKeyboard() const noexcept;

private:
    UPtr<ImGuiRenderer> mRenderer;
};
