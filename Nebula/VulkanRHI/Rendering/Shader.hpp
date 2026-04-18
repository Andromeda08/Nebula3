#pragma once

#include <map>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VulkanRHI/Device.hpp"

namespace RHI
{
    struct ShaderInfo
    {
        std::filesystem::path   filePath;
        vk::ShaderStageFlagBits shaderStage;
        const char*             entryPoint = "main";
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
        static std::vector<char> readShaderFile(const std::filesystem::path& filePath);

        static CompiledShader compileShader(const Device* pDevice, const ShaderInfo& shaderInfo);

        static CompiledShaders compileShaders(const Device* pDevice, std::map<vk::ShaderStageFlagBits, ShaderInfo>& shaderInfos);
    };
}
