#include "Shader.hpp"

#include <fstream>
#include <ranges>
#include <vector>

namespace RHI
{
    std::vector<vk::PipelineShaderStageCreateInfo> getStageCreateInfos(const CompiledShaders& compiledShaders)
    {
        return compiledShaders
            | std::views::transform([](const auto& x){ return x.second.shaderStageInfo; })
            | std::ranges::to<std::vector<vk::PipelineShaderStageCreateInfo>>();
    }

    std::vector<char> Shader::readShaderFile(const char* filePath)
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        assert(file.is_open());

        const std::streamsize fileSize = file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    CompiledShaders Shader::compileShaders(const Device* pDevice, std::map<vk::ShaderStageFlagBits, ShaderInfo>& shaderInfos)
    {
        CompiledShaders result;
        for (const auto& [stage, shaderInfo] : shaderInfos)
        {
            const auto shaderSrc = readShaderFile(shaderInfo.filePath);
            auto shaderModuleCreateInfo = vk::ShaderModuleCreateInfo()
                .setCodeSize(sizeof(char) * shaderSrc.size())
                .setPCode(reinterpret_cast<const uint32_t*>(shaderSrc.data()));

            const auto shaderModule = pDevice->getHandle().createShaderModule(shaderModuleCreateInfo);
            result[stage] = {
                .shaderInfo      = shaderInfo,
                .shaderModule    = shaderModule,
                .shaderStageInfo = vk::PipelineShaderStageCreateInfo()
                    .setStage(stage)
                    .setModule(shaderModule)
                    .setPName(shaderInfo.entryPoint),
            };

            pDevice->nameObject<vk::ShaderModule>({
                .debugName = shaderInfo.filePath,
                .handle    = shaderModule,
            });
        }
        return result;
    }
}
