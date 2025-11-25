#pragma once

#include <vulkan/vulkan.hpp>

#include "Pipeline.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "VulkanRHI/Device.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    struct ComputePipelineCreateInfo : public PipelineCreateInfo
    {
    };

    class ComputePipeline final : public Pipeline
    {
    public:
        nbl_DISABLE_COPY(ComputePipeline);
        explicit ComputePipeline(ComputePipelineCreateInfo& createInfo);

        static UPtr<ComputePipeline> create(ComputePipelineCreateInfo& createInfo);

        ~ComputePipeline() override = default;
    };
}
