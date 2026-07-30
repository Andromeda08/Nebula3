#pragma once
#include "VulkanRHI/Frame.hpp"
#include "VulkanRHI/Commands/CommandList.hpp"

namespace nbl
{
    struct HairRenderer_BDAs
    {
        uint64_t cameraBuffer;
        uint64_t lightsBuffer;
    };

    class IHairRenderer
    {
    public:
        virtual ~IHairRenderer() = default;

        virtual void execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const HairRenderer_BDAs& buffers) = 0;

        virtual void onDrawUI() {}

        virtual const SPtr<RHI::Image>& getResult(uint32_t frameIndex) const = 0;
    };
}
