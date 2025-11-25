#include "UserInterface.hpp"

#include <imgui.h>

UserInterface::UserInterface(const UserInterfaceCreateInfo& createInfo)
{
    mRenderer = ImGuiRenderer::create({
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

void UserInterface::draw(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const
{
    mRenderer->render(pCommandList->getHandle(), frameData, [&]()
    {
        for (const auto& component : mComponents)
        {
            component->draw();
        }
    });
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
