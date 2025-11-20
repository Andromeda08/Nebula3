#pragma once

#include <filesystem>
#include <fstream>
#include <print>
#include <nlohmann/json.hpp>

#include "Core/AppSpecification.hpp"
#include "Core/Types.hpp"
#include "VulkanRHI/RHIConfiguration.hpp"

constexpr auto gConfigurationPath = "nbl.json";

/**
 * All configuration objects must be (de)serializable eg. via "NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE" and have default values.
 */
struct ConfigurationData
{
    AppSpecification app = {};
    RHIConfiguration rhi = {};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConfigurationData, app, rhi);

class Configuration
{
public:
    static Configuration* getInstance()
    {
        if (!sInstance)
        {
            sInstance = UPtr<Configuration>(new Configuration());
        }
        return sInstance.get();
    }

    static const ConfigurationData& getConfig()
    {
        return sInstance->mData;
    }

private:
    Configuration()
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

    void writeConfigFile() const
    {
        std::ofstream configFile(gConfigurationPath);
        assert(configFile.is_open());

        auto json = nlohmann::json({});
        to_json(json, mData);
        configFile << json;
        configFile.close();
    }

    void readConfigFile()
    {
        std::fstream configFile(gConfigurationPath);
        assert(configFile.is_open());
        const auto json = nlohmann::json::parse(configFile);
        configFile.close();
        from_json(json, mData);
    }

    static UPtr<Configuration> sInstance;

    ConfigurationData mData;
};

UPtr<Configuration> Configuration::sInstance = nullptr;
