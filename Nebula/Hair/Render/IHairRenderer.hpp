#pragma once
#include "VulkanRHI/Frame.hpp"
#include "VulkanRHI/Commands/CommandList.hpp"

namespace nbl
{
    class IHairRenderer
    {
    public:
        virtual ~IHairRenderer() = default;

        virtual void execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, uint64_t cameraBufferAddress) = 0;

        virtual const SPtr<RHI::Image>& getResult(uint32_t frameIndex) const = 0;
    };
}
