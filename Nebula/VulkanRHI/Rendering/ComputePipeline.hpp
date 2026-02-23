#pragma once

#include <vulkan/vulkan.hpp>

#include "Pipeline.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "VulkanRHI/Device.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    begin_PipelineCreateInfoStruct(Compute)
        ShaderInfo computeShader;

        ComputePipelineCreateInfo& setComputeShader(const ShaderInfo& shaderInfo)
        {
            assert(shaderInfo.shaderStage == vk::ShaderStageFlagBits::eCompute);
            computeShader = shaderInfo;
            return *this;
        }

        ComputePipelineCreateInfo& setComputeShader(const std::string& shaderPath)
        {
            computeShader = {
                .filePath    = shaderPath,
                .shaderStage = vk::ShaderStageFlagBits::eCompute,
                .entryPoint  = "main"
            };
            return *this;
        }
    end_PipelineCreateInfoStruct;

    class ComputePipeline final : public Pipeline
    {
    public:
        nbl_DISABLE_COPY(ComputePipeline);
        explicit ComputePipeline(ComputePipelineCreateInfo& createInfo);

        static UPtr<ComputePipeline> create(ComputePipelineCreateInfo& createInfo);

        ~ComputePipeline() override = default;

        void dispatch(const vk::CommandBuffer& commandBuffer, uint32_t sizeX = 1, uint32_t sizeY = 1, uint32_t sizeZ = 1) const;

        void dispatch(const CommandList* pCommandList, uint32_t sizeX = 1, uint32_t sizeY = 1, uint32_t sizeZ = 1) const noexcept;
    };
}
