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
        for (const auto& [i, pass] : nbl::enumerate(mPasses))
        {
            pCommandList->beginLabel(mLabels[i].color, mLabels[i].name);

            if (mBarriers.contains(i))
            {
                mBarriers.at(i).insert(pCommandList);
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
