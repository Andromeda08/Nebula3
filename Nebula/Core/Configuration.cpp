#include "Configuration.hpp"

#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

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

std::filesystem::path Configuration::getSceneFilePath(const std::string& sceneFile) noexcept
{
    const auto& config = Configuration::getConfig();
    return std::format("{}/{}/{}", config.baseDirPath, config.scenesDirName, sceneFile);
}

std::filesystem::path Configuration::getFontFilePath(const std::string& fontFile) noexcept
{
    const auto& config = Configuration::getConfig();
    return std::format("{}/{}/{}", config.baseDirPath, config.fontsDirName, fontFile);
}

std::filesystem::path Configuration::getShaderFilePath(const std::string& shaderFile) noexcept
{
    const auto& config = getConfig();
    return std::format("{}/{}/{}", config.baseDirPath, config.shadersBinDirName, shaderFile);
}

std::filesystem::path Configuration::getShaderSourceFilePath(const std::string& shaderFile) noexcept
{
    const auto& config = Configuration::getConfig();
    return std::format("{}/{}/{}", config.baseDirPath, config.shadersDirName, shaderFile);
}

std::filesystem::path Configuration::getShaderSourceDir() noexcept
{
    const auto& config = Configuration::getConfig();
    return std::format("{}/{}", config.baseDirPath, config.shadersDirName);
}

std::filesystem::path Configuration::getShaderDir() noexcept
{
    const auto& config = Configuration::getConfig();
    return std::format("{}/{}", config.baseDirPath, config.shadersBinDirName);
}

std::filesystem::path Configuration::getTextureFilePath(const std::string& textureFile) noexcept
{
    const auto& config = Configuration::getConfig();
    return std::format("{}/{}/{}", config.baseDirPath, config.texturesDirName, textureFile);
}

std::filesystem::path Configuration::getHairDir() noexcept
{
    const auto& config = Configuration::getConfig();
    return std::format("{}/{}", config.baseDirPath, config.hairDirName);
}

std::filesystem::path Configuration::getHairFilePath(const std::string& hairFile) noexcept
{
    const auto& config = Configuration::getConfig();
    return std::format("{}/{}/{}", config.baseDirPath, config.hairDirName, hairFile);
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
        spdlog::debug("[Config] Regenerating configuration from in-code defaults. (dev-mode)");
        #else
        spdlog::warn("[Config] No configuration file found, using defaults and creating one.");
        #endif
        return;
    }

    readConfigFile();
    spdlog::debug("[Config] Configuration loaded from file.");
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
