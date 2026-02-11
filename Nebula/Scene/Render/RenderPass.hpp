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
        mScissor = RHI2::Rect2D()
            .setExtent({ mRenderResolution.width, mRenderResolution.height })
            .setOffset({ 0, 0 });

        mViewport = RHI2::Viewport()
            .setX(0.0f)
            .setY(0.0f)
            .setWidth(mRenderResolution.width)
            .setHeight(mRenderResolution.height)
            .setMinDepth(0.0f)
            .setMaxDepth(1.0f);
    }

    virtual ~RenderPass() = default;

    virtual void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept = 0;

protected:
    void setScissorViewport(const RHI::CommandList* pCommandList) const noexcept
    {
        pCommandList->setScissor(mScissor);
        pCommandList->setViewport(mViewport);
    }

    [[nodiscard]] RHI2::Rect2D getRenderArea() const noexcept
    {
        return mScissor;
    }

    vk::Extent2D         mRenderResolution;

    // Cache Scissor & Viewport
    RHI2::Rect2D         mScissor;
    RHI2::Viewport       mViewport;

    SPtr<RHI::VulkanRHI> mRHI;
    std::string          mName;
};
