#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "RHI/RHI.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    class CommandList;

    struct Attachment
    {
        vk::Image                   image;
        vk::RenderingAttachmentInfo attachmentInfo;
    };

    struct RenderPassCreateInfo
    {
        RHI2::Rect2D            renderArea = {{0, 0}, {0, 0}};
        std::vector<Attachment> colorAttachments;
        Attachment              depthAttachment;
        std::string             label = "Unknown Pass";
    };

    class RenderPass
    {
    public:
        nbl_DISABLE_COPY(RenderPass);
        nbl_CTOR(RenderPass);

        void execute(const vk::CommandBuffer& commandList, const std::function<void(const vk::CommandBuffer&)>& lambda) const;

        void execute(const CommandList* pCommandList, const std::function<void(const CommandList*)>& lambda) const;

        void setColorAttachment(uint32_t index, const Attachment& attachment);

    private:
        vk::RenderingInfo                        mRenderingInfo;
        vk::Rect2D                               mRenderArea;

        std::vector<Attachment>                  mColorAttachments;
        std::vector<vk::RenderingAttachmentInfo> mColorRenderingInfos;
        Attachment                               mDepthAttachment;

        std::string                              mLabel;
    };
}
