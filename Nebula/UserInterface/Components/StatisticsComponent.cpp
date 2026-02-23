#include "StatisticsComponent.hpp"

#include <imgui.h>

#include "Core/Types.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

StatisticsComponent::StatisticsComponent(const SPtr<RHI::VulkanRHI>& rhi, float* pCPUFramerate)
: mRHI(rhi)
, mCPUFramerate(pCPUFramerate)
{

}

void StatisticsComponent::draw()
{
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin("Statistics");
    ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
    ImGui::Text("GPU: %s", mRHI->getDevice()->getDeviceName().c_str());
    ImGui::Text("CPU Time: %.4f ms", *mCPUFramerate);
    ImGui::End();
}
