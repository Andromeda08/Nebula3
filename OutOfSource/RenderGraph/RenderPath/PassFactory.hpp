#pragma once

#include "Core/Types.hpp"
#include "RenderPass/Pass.hpp"

namespace RHI
{
    class VulkanRHI;
}

namespace rg
{
    class Node;

    class PassFactory
    {
    public:
        explicit PassFactory(const SPtr<RHI::VulkanRHI>& rhi);

        UPtr<Pass> create(const Node* pNode) const;

    private:
        SPtr<RHI::VulkanRHI> mRHI;
    };
}
