#pragma once

#include <functional>
#include "Core/Macro.hpp"
#include "VulkanRHI/VulkanRHI.hpp"
#include "Window/SDLWindow.hpp"

struct ImGuiRenderPassCreateInfo
{
    std::string          fontFile;
    SPtr<SDLWindow>      window;
    SPtr<RHI::VulkanRHI> rhi;
};

class ImGuiRenderPass
{
public:
    nbl_DISABLE_COPY(ImGuiRenderPass);
    nbl_CTOR(ImGuiRenderPass);

    ~ImGuiRenderPass();

    void render(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData,
        const std::function<void()>& uiDraws) noexcept;

private:
    void init_VulkanResources() noexcept;
    void init_ImGui() const noexcept;

    // Vulkan resources for ImGui
    vk::DescriptorPool      mDescriptorPool;
    vk::PipelineCache       mPipelineCache;
    vk::RenderingInfo       mRenderingInfo;
    vk::DebugUtilsLabelEXT  mDebugLabel;

    const std::string       mFontFile;
    SPtr<SDLWindow>         mWindow;
    SPtr<RHI::VulkanRHI>    mRHI;
    SPtr<RHI::Device>       mDevice;
};