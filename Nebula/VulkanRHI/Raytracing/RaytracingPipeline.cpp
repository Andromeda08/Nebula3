#include "RaytracingPipeline.hpp"

#include <ranges>

namespace RHI
{
    RaytracingPipeline::RaytracingPipeline(const RaytracingPipelineCreateInfo& createInfo)
    : Pipeline()
    , mRayDepth(createInfo.rayDepth)
    , mDebugName(createInfo.debugName)
    {
        mPushConstantRange = createInfo.pushConstantRange;
        mDevice = createInfo.device;
        mPipelineType = PipelineType::RayTracing;
        mBindPoint = PipelineUtils::pipelineTypeToBindPoint(mPipelineType);

        auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
            .setSetLayoutCount(createInfo.descriptorSetLayouts.size())
            .setPSetLayouts(createInfo.descriptorSetLayouts.data())
            .setPushConstantRangeCount(mPushConstantRange.size != 0 ? 1 : 0)
            .setPPushConstantRanges(mPushConstantRange.size != 0 ? &mPushConstantRange : nullptr);

        mPipelineLayout = mDevice->getHandle().createPipelineLayout(layoutCreateInfo);
        mDevice->nameObject<vk::PipelineLayout>({
            .debugName = std::format("{} Layout", createInfo.debugName),
            .handle    = mPipelineLayout,
        });

        ShaderBindingTableCreateInfo shaderBindingTableCreateInfo;

        bool hasRaygenShader = false;
        std::vector<CompiledShader> compiledShaders;
        for (const auto& shaderInfo : createInfo.raytracingShaderInfos)
        {
            compiledShaders.push_back(Shader::compileShader(mDevice.get(), shaderInfo));

            if (shaderInfo.shaderStage == vk::ShaderStageFlagBits::eRaygenKHR)
            {
                if (hasRaygenShader) assert(false);
                hasRaygenShader = true;
            }

            if (shaderInfo.shaderStage == vk::ShaderStageFlagBits::eMissKHR)
            {
                shaderBindingTableCreateInfo.missCount++;
            }
            if (shaderInfo.shaderStage == vk::ShaderStageFlagBits::eClosestHitKHR)
            {
                shaderBindingTableCreateInfo.hitCount++;
            }
            if (shaderInfo.shaderStage == vk::ShaderStageFlagBits::eCallableKHR)
            {
                shaderBindingTableCreateInfo.callableCount++;
            }
        }

        const auto pipelineStageCreateInfos = compiledShaders
            | std::views::transform([](const auto& x){ return x.shaderStageInfo; })
            | std::ranges::to<std::vector<vk::PipelineShaderStageCreateInfo>>();

        assert(hasRaygenShader);
        const auto shaderGroups = createShaderGroups(pipelineStageCreateInfos);
        const auto raytracingPipelineCreateInfo = vk::RayTracingPipelineCreateInfoKHR()
            .setFlags(vk::PipelineCreateFlagBits::eRayTracingNoNullClosestHitShadersKHR | vk::PipelineCreateFlagBits::eRayTracingNoNullMissShadersKHR)
            .setStageCount(pipelineStageCreateInfos.size())
            .setPStages(pipelineStageCreateInfos.data())
            .setGroupCount(shaderGroups.size())
            .setPGroups(shaderGroups.data())
            .setMaxPipelineRayRecursionDepth(mRayDepth)
            .setLayout(mPipelineLayout);

        const vk::Result result = mDevice->getHandle().createRayTracingPipelinesKHR(nullptr, nullptr, 1, &raytracingPipelineCreateInfo, nullptr, &mPipeline);
        if (result != vk::Result::eSuccess)
        {
            exitWithError("Failed to create RT pipeline: {}", vk::to_string(result));
        }

        mDevice->nameObject<vk::Pipeline>({
            .debugName = createInfo.debugName,
            .handle    = mPipeline,
        });

        shaderBindingTableCreateInfo.pipeline  = mPipeline;
        shaderBindingTableCreateInfo.device    = mDevice;
        shaderBindingTableCreateInfo.debugName = std::format("{} [SBT]", mDebugName);

        mShaderBindingTable = ShaderBindingTable::create(shaderBindingTableCreateInfo);
    }

    void RaytracingPipeline::traceRays(const vk::CommandBuffer& commandBuffer, const uint32_t sizeX, const uint32_t sizeY) const
    {
        commandBuffer.traceRaysKHR(
            mShaderBindingTable->getRaygenRegion(), mShaderBindingTable->getMissRegion(),
            mShaderBindingTable->getHitRegion(),mShaderBindingTable->getCallRegion(),
            sizeX, sizeY, mRayDepth);
    }

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> RaytracingPipeline::createShaderGroups(
        const std::vector<vk::PipelineShaderStageCreateInfo>& stageCreateInfos) const
    {
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> result;
        for (auto i = 0; i < stageCreateInfos.size(); i++)
        {
            switch (stageCreateInfos[i].stage)
            {
                case vk::ShaderStageFlagBits::eRaygenKHR:
                case vk::ShaderStageFlagBits::eMissKHR: {
                    auto group = vk::RayTracingShaderGroupCreateInfoKHR()
                        .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
                        .setGeneralShader(i)
                        .setClosestHitShader(VK_SHADER_UNUSED_KHR)
                        .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                        .setIntersectionShader(VK_SHADER_UNUSED_KHR);
                    result.push_back(group);
                    break;
                }
                case vk::ShaderStageFlagBits::eClosestHitKHR: {
                    auto group = vk::RayTracingShaderGroupCreateInfoKHR()
                        .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
                        .setGeneralShader(VK_SHADER_UNUSED_KHR)
                        .setClosestHitShader(i)
                        .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                        .setIntersectionShader(VK_SHADER_UNUSED_KHR);
                    result.push_back(group);
                    break;
                }
                case vk::ShaderStageFlagBits::eCallableKHR: {
                    auto group = vk::RayTracingShaderGroupCreateInfoKHR()
                        .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
                        .setGeneralShader(i)
                        .setClosestHitShader(VK_SHADER_UNUSED_KHR)
                        .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                        .setIntersectionShader(VK_SHADER_UNUSED_KHR);
                    result.push_back(group);
                    break;
                }
                default: {
                    std::println("[RHI] Warning: Non-Raytracing shader ({}) specified for Pipeline {}", to_string(stageCreateInfos[i].stage), mDebugName);
                }
            }
        }
        return result;
    }
}
