#pragma once

#include <map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VulkanRHI/Device.hpp"

namespace RHI
{
    struct ShaderInfo
    {
        const char*             filePath;
        const char*             entryPoint;
        vk::ShaderStageFlagBits shaderStage;
    };

    struct CompiledShader
    {
        ShaderInfo                        shaderInfo;
        vk::ShaderModule                  shaderModule;
        vk::PipelineShaderStageCreateInfo shaderStageInfo;
    };
    using CompiledShaders = std::map<vk::ShaderStageFlagBits, CompiledShader>;

    std::vector<vk::PipelineShaderStageCreateInfo> getStageCreateInfos(const CompiledShaders& compiledShaders);

    struct Shader
    {
        static std::vector<char> readShaderFile(const char* filePath);

        static CompiledShaders compileShaders(const Device* pDevice, std::map<vk::ShaderStageFlagBits, ShaderInfo>& shaderInfos);
    };
}
