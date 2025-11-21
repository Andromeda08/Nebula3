#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    struct Attachment
    {
        SPtr<Image>                 image;
        vk::RenderingAttachmentInfo attachmentInfo;
    };

    struct RenderPassCreateInfo
    {
        vk::Rect2D              renderArea;
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
