#include "StatisticsComponent.hpp"

#include <imgui.h>

void StatisticsComponent::draw()
{
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin("Statistics");
    ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
    ImGui::End();
}
