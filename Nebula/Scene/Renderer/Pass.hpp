#pragma once
#include "VulkanRHI/Frame.hpp"
#include "VulkanRHI/VulkanRHI.hpp"
#include "VulkanRHI/Commands/CommandList.hpp"

namespace nbl
{
    struct PassParams
    {
        SPtr<RHI::VulkanRHI> rhi;
        std::string          name;
    };

    class Pass
    {
    public:
        explicit Pass(const PassParams& params)
        : mRHI(params.rhi)
        , mName(params.name)
        {
        }

        virtual ~Pass() = default;

        virtual void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) = 0;

        [[nodiscard]] const std::string& getName() const noexcept
        {
            return mName;
        }

    protected:
        SPtr<RHI::VulkanRHI> mRHI;
        std::string          mName;
    };

    struct RenderPassParams
    {
        vk::Extent2D    extent;
        PassParams      base;
    };

    class RenderPass : public Pass
    {
    public:
        explicit RenderPass(const RenderPassParams& params)
        : Pass(params.base)
        , mRenderResolution(params.extent)
        {
            mScissor  = vk::Rect2D {{ 0, 0 }, mRenderResolution };
            mViewport = vk::Viewport { 0.0f, 0.0f, static_cast<float>(mRenderResolution.width), static_cast<float>(mRenderResolution.height), 0.0f, 1.0f };
        }

        ~RenderPass() override = default;

        void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override
        {
            pCommandList->beginLabel(mName);
            renderPass(pCommandList, frameData);
            pCommandList->endLabel();
        }

        [[nodiscard]] virtual const SPtr<RHI::Image>& getResult() const noexcept = 0;

    protected:
        [[nodiscard]] SPtr<RHI::Image> makeRenderTarget(const std::string& name, const vk::Format format = vk::Format::eR32G32B32A32Sfloat) const
        {
            using enum vk::ImageUsageFlagBits;
            vk::ImageUsageFlags usageFlags = eSampled | eTransferSrc | eTransferDst;

            if (vk::hasDepthComponent(format))
            {
                usageFlags |= eDepthStencilAttachment;
            }
            else
            {
                usageFlags |= eColorAttachment | eStorage;
            }

            return mRHI->createImage({
                .extent     = mRenderResolution,
                .format     = format,
                .usageFlags = usageFlags,
                .debugName  = std::format("{}_{}", mName, name),
            });
        }

        virtual void renderPass(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) = 0;

        void setFullAreaViewportScissor(const RHI::CommandList* pCommandList) const noexcept
        {
            pCommandList->setViewportScissor(mViewport, mScissor);
        }

        [[nodiscard]] static RHI::Attachment makeColorAttachment(const SPtr<RHI::Image>& image)
        {
            RHI::Attachment attachment {};

            attachment.image          = image->getImage();
            attachment.attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setColor({ 0.0f, 0.0f, 0.0f, 1.0f }))
                .setImageView(image->getImageView())
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore);

            return attachment;
        }

        [[nodiscard]] static RHI::Attachment makeDepthAttachment(const SPtr<RHI::Image>& image)
        {
            RHI::Attachment attachment {};

            attachment.image          = image->getImage();
            attachment.attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setDepthStencil({ 1.0f, 0 }))
                .setImageView(image->getImageView())
                .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore);

            return attachment;
        }

        vk::Extent2D                                mRenderResolution;
        vk::Rect2D                                  mScissor;
        vk::Viewport                                mViewport;
    };
}
