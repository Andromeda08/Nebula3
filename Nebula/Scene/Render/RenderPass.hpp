#pragma once

#include "VulkanRHI/VulkanRHI.hpp"

struct RenderPass_Params
{
    Size2D               renderResolution;
    SPtr<RHI::VulkanRHI> rhi;
    std::string          name;
};

class RenderPass
{
public:
    explicit RenderPass(const RenderPass_Params& params)
    : mRenderResolution(vk::Extent2D { params.renderResolution.width, params.renderResolution.height })
    , mRHI(params.rhi)
    , mName(params.name)
    {
        mScissor = vk::Rect2D()
            .setExtent(mRenderResolution)
            .setOffset({ 0, 0 });

        mViewport = vk::Viewport()
            .setX(0.0f)
            .setY(0.0f)
            .setWidth(mRenderResolution.width)
            .setHeight(mRenderResolution.height)
            .setMinDepth(0.0f)
            .setMaxDepth(1.0f);
    }

    virtual ~RenderPass() = default;

    virtual void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept = 0;

    virtual SPtr<RHI::Image> getResult() const noexcept
    {
        return nullptr;
    }

protected:
    void setScissorViewport(const RHI::CommandList* pCommandList) const noexcept
    {
        const auto& handle = pCommandList->getHandle();
        handle.setScissor(0, mScissor);
        handle.setViewport(0, mViewport);
    }

    [[nodiscard]] vk::Rect2D getRenderArea() const noexcept
    {
        return mScissor;
    }

    vk::Extent2D         mRenderResolution;

    // Cache Scissor & Viewport
    vk::Rect2D           mScissor;
    vk::Viewport         mViewport;

    SPtr<RHI::VulkanRHI> mRHI;
    std::string          mName;
};
