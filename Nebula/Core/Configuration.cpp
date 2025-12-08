#include "Configuration.hpp"

#include <filesystem>
#include <fstream>
#include <print>

UPtr<Configuration> Configuration::sInstance = nullptr;

Configuration* Configuration::getInstance()
{
    if (!sInstance)
    {
        sInstance = UPtr<Configuration>(new Configuration());
    }
    return sInstance.get();
}

ConfigurationData& Configuration::getConfig()
{
    return sInstance->mData;
}

std::string Configuration::getShaderFilePath(const std::string& shaderFile) noexcept
{
    return std::format("{}/{}", sInstance->mData.app.shadersDir, shaderFile);
}

std::string Configuration::getTextureFilePath(const std::string& textureFile) noexcept
{
    return std::format("{}/{}", sInstance->mData.scenes.texturesDir, textureFile);
}

Configuration::Configuration()
{
    #ifdef NBL_CONFIG_REGEN
    constexpr auto regenConfigFile = true;
    #else
    const auto regenConfigFile = !std::filesystem::exists(gConfigurationPath);
    #endif

    if (regenConfigFile)
    {
        mData = ConfigurationData();
        writeConfigFile();
        #ifdef NBL_CONFIG_REGEN
        std::println("[Config] Regenerating configuration from in-code defaults. (dev-mode)");
        #else
        std::println("[Config] No configuration file found, using defaults and creating one.");
        #endif
        return;
    }

    readConfigFile();
    std::println("[Config] Configuration loaded from file.");
}

void Configuration::writeConfigFile() const
{
    std::ofstream configFile(gConfigurationPath);
    assert(configFile.is_open());

    auto json = nlohmann::json({});
    to_json(json, mData);
    configFile << json;
    configFile.close();
}

void Configuration::readConfigFile()
{
    std::fstream configFile(gConfigurationPath);
    assert(configFile.is_open());
    const auto json = nlohmann::json::parse(configFile);
    configFile.close();
    from_json(json, mData);
}
