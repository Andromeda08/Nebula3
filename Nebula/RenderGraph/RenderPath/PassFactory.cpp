#include "PassFactory.hpp"

#include <format>
#include <stdexcept>
#include <string>

#include "RenderGraph/Node.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

#include "RenderPass/Molecule/SDFComputePass.hpp"
#include "RenderPass/Special/ScenePass.hpp"

namespace rg
{
    PassFactory::PassFactory(const SPtr<RHI::VulkanRHI>& rhi)
    : mRHI(rhi)
    {
    }

    // [AddNode-3] Add to PassFactory
    UPtr<Pass> PassFactory::create(const Node* pNode) const
    {
        switch (pNode->getNodeType())
        {
            case NodeType::Scene: {
                return makeUnique<ScenePass>();
            }
            default: {
                throw std::runtime_error(std::format("[PassFactory] Error: NodeType {} is not implemented", toString(pNode->getNodeType())));
            }
        }
    }
}
