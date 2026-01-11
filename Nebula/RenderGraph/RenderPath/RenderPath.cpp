#include "RenderPath.hpp"

#include "PassFactory.hpp"

namespace rg
{
    RenderPath::RenderPath(const RenderPathCreateInfo& createInfo)
    : mRenderGraphName(createInfo.compilerResult.inputGraphName)
    {
        // =====================================
        // Create Resources & Barriers
        // =====================================
        const auto resourceFactory = ResourceFactory(createInfo.rhi);
        for (const auto& resourceTemplate : createInfo.compilerResult.resourceTemplates)
        {
            // Create resource
            const auto resource = resourceFactory.create(resourceTemplate);
            mResources[resource->getName()] = resource;

            // Barriers : Images
            if (const auto* pImageResource = resource->as<ImageResource>())
            {
                // Initializer barrier
                const auto globalFirstUse = resourceTemplate.getFirstUsagePoint();
                if (!globalFirstUse.has_value())
                {
                    continue;
                }

                const auto& resourceParams = globalFirstUse.value().dependencyInfo.resourceParams;
                if (const auto* params = std::get_if<ImageInfo>(&resourceParams))
                {
                    mInitialResourceBarriers.addImageBarrier({
                        .dstUsage = params->imageUsage,
                        .image    = pImageResource->getImage(),
                    });
                }

                // Per execution point barriers
                for (const auto& usageRange : resourceTemplate.usageRanges)
                {
                    const auto localFirstUse = resourceTemplate.getUsagePoint(usageRange.start);
                    if (!localFirstUse.has_value())
                    {
                        continue;
                    }

                    const auto& localParams = localFirstUse.value().dependencyInfo.resourceParams;
                    if (const auto* params = std::get_if<ImageInfo>(&localParams))
                    {
                        mBarriers[usageRange.start].addImageBarrier({
                            .dstUsage = params->imageUsage,
                            .image    = pImageResource->getImage(),
                        });
                    }
                }
            }
        }

        // =====================================
        // Create Passes & Connect Resources
        // =====================================
        const auto passFactory = PassFactory(createInfo.rhi);
        std::map<int32_t, int32_t> nodeMapping; // Graph ID -> mPasses Index
        for (const auto* node : createInfo.compilerResult.nodeExecutionOrder)
        {
            // Debug label
            const auto& c = Configuration::getConfig().renderGraph.getNodeStyle(node->getNodeType()).cTitleBar;
            mLabels.push_back({
                .color = { c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f },
                .name  = node->getSubTitle().empty() ? node->getDisplayName() : std::format("{} ({})", node->getDisplayName(), node->getSubTitle()),
            });

            // Create Pass
            mPasses.push_back(passFactory.create(node));
            nodeMapping.insert_or_assign(node->getId(), mPasses.size() - 1);

            // TODO : Connect resources
            for (const auto& optResource : createInfo.compilerResult.resourceTemplates)
            {
                // 6.0 Get resource
                const auto& resource = mResources[std::to_string(optResource.id)];

                // 6.1 Connect to origin node
                auto& origin = optResource.originalResource;
                auto& cnode_origin = mPasses[nodeMapping[origin.pNode->getId()]];
                cnode_origin->setResource(origin.originalDepName, resource);

                // 6.2 Connect to consumer nodes
                for (const auto& consumer : optResource.usagePoints)
                {
                    auto& cnode_consumer = mPasses[nodeMapping[consumer.userNodeId]];
                    cnode_consumer->setResource(consumer.usedAs, resource);
                }
            }
        }
    }

    void RenderPath::initialize(const RHI::CommandList* pCommandList)
    {
        if (mIsInitialized)
        {
            return;
        }

        mInitialResourceBarriers.insert(pCommandList);
        mIsInitialized = true;
    }

    void RenderPath::update(float dt, const RHI::FrameData& frameData)
    {
        for (auto&& pass : mPasses)
        {
            pass->update();
        }
    }

    void RenderPath::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        for (const auto& [i, pass] : std::views::enumerate(mPasses))
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
}
