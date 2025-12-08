#pragma once

#include <nlohmann/json.hpp>

#include "Macro.hpp"
#include "Core/AppSpecification.hpp"
#include "Core/Types.hpp"
#include "RenderGraph/RGConfiguration.hpp"
#include "Scene/SceneConfiguration.hpp"
#include "VulkanRHI/RHIConfiguration.hpp"

constexpr auto gConfigurationPath = "nbl.json";

/**
 * All configuration objects must be (de)serializable eg. via "NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE" and have default values.
 */
struct ConfigurationData
{
    uint32_t            version     = 1u;  // ConfigurationData version tag
    AppSpecification    app         = {};
    RHIConfiguration    rhi         = {};
    RGConfiguration     renderGraph = {};
    SceneConfiguration  scenes      = {};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConfigurationData, version, app, rhi, renderGraph);

/**
 * Configuration reading & loading (singleton) class.
 * [Lifetime] Created and initialized first on program start, valid until the program is closed.
 */
class Configuration
{
public:
    nbl_DISABLE_COPY(Configuration);

    // Get Configuration instance.
    static Configuration* getInstance();

    // Directly get the configuration data.
    static ConfigurationData& getConfig();

    // Quick-access utilities
    static std::string getShaderFilePath(const std::string& shaderFile) noexcept;

    static std::string getTextureFilePath(const std::string& textureFile) noexcept;

private:
    Configuration();

    void writeConfigFile() const;

    void readConfigFile();

    static UPtr<Configuration> sInstance;

    ConfigurationData mData;
};
