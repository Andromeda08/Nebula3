#include "RenderPath.hpp"

#include "PassFactory.hpp"

namespace rg
{
    RenderPath::RenderPath(const RenderPathCreateInfo& createInfo)
    : mRenderGraphName(createInfo.compilerResult.inputGraphName)
    {
        // TODO: How to propagate RGCtx? Probably not the way to go anyway.
        mResourceRegistry = makeUnique<ResourceRegistry>(nullptr, createInfo.rhi);
        createResources(createInfo);
    }

    void RenderPath::initialize(const RHI::CommandList* pCommandList)
    {
        if (mIsInitialized)
        {
            return;
        }

        old_mInitialResourceBarriers.insert(pCommandList);
        mIsInitialized = true;
    }

    void RenderPath::update(float dt, const RHI::FrameData& frameData)
    {
        for (auto&& pass : old_mPasses)
        {
            pass->update();
        }
    }

    void RenderPath::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        for (const auto& [i, pass] : nbl::enumerate(old_mPasses))
        {
            pCommandList->beginLabel(old_mLabels[i].color, old_mLabels[i].name);

            if (old_mBarriers.contains(i))
            {
                old_mBarriers.at(i).insert(pCommandList);
            }

            pass->execute(pCommandList, frameData);

            pCommandList->endLabel();
        }
    }

    void RenderPath::createResources(const RenderPathCreateInfo& createInfo) noexcept
    {
        for (const auto& resourceTemplate : createInfo.compilerResult.resourceTemplates)
        {
            mResourceRegistry->create(resourceTemplate);
        }
    }
}
