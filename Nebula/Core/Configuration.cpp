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

const ConfigurationData& Configuration::getConfig()
{
    return sInstance->mData;
}

Configuration::Configuration()
{
    const auto hasConfigFile = std::filesystem::exists(gConfigurationPath);
    if (!hasConfigFile)
    {
        mData = ConfigurationData();
        writeConfigFile();
        std::println("[Config] No configuration file found, using defaults and creating one.");
    }
    else
    {
        readConfigFile();
        std::println("[Config] Configuration loaded from file.");
    }
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
