#pragma once

#include "Core/Types.hpp"
#include "RenderPass/IPass.hpp"

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

        UPtr<IPass> create(const Node* pNode) const;

    private:
        SPtr<RHI::VulkanRHI> mRHI;
    };
}
