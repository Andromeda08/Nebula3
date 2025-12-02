#pragma once

#include "VulkanRHI/Frame.hpp"
#include "VulkanRHI/Commands/CommandList.hpp"

namespace rg
{
    class Resource;
}

class IPass
{
public:
    virtual ~IPass() = default;

    virtual void update() {}
    virtual void execute(const RHI::CommandList* commandBuffer, const RHI::FrameData& frameData) = 0;

    virtual void setResource(const std::string& name, const SPtr<rg::Resource>& resource) {}
    virtual rg::Resource* getResource(const std::string& name)
    {
        return nullptr;
    }
};
