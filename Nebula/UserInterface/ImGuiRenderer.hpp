#pragma once

#include <vulkan/vulkan.hpp>

#include "Core/Types.hpp"
#include "VulkanRHI/VulkanRHI.hpp"
#include "Window/Window.hpp"

struct ImGuiRendererCreateInfo
{
    std::string          fontFile;
    SPtr<Window>         window;
    SPtr<RHI::VulkanRHI> rhi;
};

class ImGuiRenderer
{
public:
    nbl_DISABLE_COPY(ImGuiRenderer);
    nbl_CTOR(ImGuiRenderer);

    ~ImGuiRenderer();

    void render(
        const vk::CommandBuffer&        commandBuffer,
        const RHI::FrameData&           frameData,
        const std::function<void()>&    uiDraws);

private:
    void createDescriptorPool();
    void createRenderPass();
    void createFramebuffer();
    void initImGuiContext() const;

    vk::DescriptorPool              mDescriptorPool;
    vk::PipelineCache               mPipelineCache;
    PerFrameArray<vk::Framebuffer>  mFramebuffers;
    vk::RenderPass                  mRenderPass;
    vk::Rect2D                      mRenderArea;
    vk::RenderPassBeginInfo         mRenderPassBeginInfo;
    vk::ClearValue                  mClearValue;
    vk::DebugUtilsLabelEXT          mDebugLabel;

    const std::string               mFontFile;
    SPtr<Window>                    mWindow;
    SPtr<RHI::VulkanRHI>            mRHI;
};
