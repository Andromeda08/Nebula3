#include "UserInterface.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>

UserInterface::UserInterface(const UserInterfaceCreateInfo& createInfo)
{
    mRenderer = ImGuiRenderPass::create({
        .fontFile = createInfo.fontFile,
        .window   = createInfo.window,
        .rhi      = createInfo.rhi,
    });
}

void UserInterface::update() const
{
    for (const auto& component : mComponents)
    {
        component->update();
    }
}

void UserInterface::processEvents(const SDL_Event& event) noexcept
{
    ImGui_ImplSDL3_ProcessEvent(&event);
}

void UserInterface::draw(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const std::function<void()>& uiDraws) const
{
    mRenderer->render(pCommandList, frameData, [&]()
    {
        uiDraws();
        for (const auto& component : mComponents)
        {
            component->draw();
        }
    });
}

bool UserInterface::wantCaptureInput() noexcept
{
    return wantCaptureMouse() || wantCaptureKeyboard();
}

bool UserInterface::wantCaptureMouse() noexcept
{
    const ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

bool UserInterface::wantCaptureKeyboard() noexcept
{
    const ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}
