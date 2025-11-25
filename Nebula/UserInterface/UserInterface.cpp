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

void UserInterface::update()
{
}

void UserInterface::draw(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const
{
    mRenderer->render(pCommandList->getHandle(), frameData, [&]()
    {
        const ImGuiIO& io = ImGui::GetIO();

        ImGui::Begin("Test Component");
        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
        ImGui::End();
    });
}

bool UserInterface::wantCaptureMouse() const noexcept
{
    const ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

bool UserInterface::wantCaptureKeyboard() const noexcept
{
    const ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}
