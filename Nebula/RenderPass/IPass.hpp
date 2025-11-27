#pragma once

#include "VulkanRHI/Frame.hpp"
#include "VulkanRHI/Commands/CommandList.hpp"

class IPass
{
public:
    virtual ~IPass() = default;

    virtual void update() {}
    virtual void execute(const RHI::CommandList* commandBuffer, const RHI::FrameData& frameData) = 0;
};
