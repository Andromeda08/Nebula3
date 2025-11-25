#pragma once

#include <vulkan/vulkan.hpp>

#include "ShaderBindingTable.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "VulkanRHI/Device.hpp"
#include "VulkanRHI/VulkanCore.hpp"
#include "VulkanRHI/Rendering/Pipeline.hpp"

namespace RHI
{
    begin_PipelineCreateInfoStruct(Raytracing)
        uint32_t                rayDepth                = 1;
        std::vector<ShaderInfo> raytracingShaderInfos   = {};

        RaytracingPipelineCreateInfo& setRayDepth(const uint32_t value)
        {
            rayDepth = value;
            return *this;
        }

        RaytracingPipelineCreateInfo& addShader(const ShaderInfo& shaderInfo)
        {
            raytracingShaderInfos.push_back(shaderInfo);
            return *this;
        }
    end_PipelineCreateInfoStruct;

    class RaytracingPipeline final : public Pipeline
    {
    public:
        nbl_DISABLE_COPY(RaytracingPipeline);
        nbl_CTOR(RaytracingPipeline);

        ~RaytracingPipeline() override = default;

        void traceRays(const vk::CommandBuffer& commandBuffer, uint32_t sizeX, uint32_t sizeY) const;

    private:
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> createShaderGroups(const std::vector<vk::PipelineShaderStageCreateInfo>& stageCreateInfos) const;

        SPtr<ShaderBindingTable> mShaderBindingTable;
        const uint32_t           mRayDepth = 1;
        const std::string        mDebugName;
    };
}
