#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>

#include "Macro.hpp"
#include "Core/AppSpecification.hpp"
#include "Core/Types.hpp"
#include "RenderGraph/RGConfiguration.hpp"

constexpr auto gConfigurationPath = "nbl.json";

/**
 * All configuration objects must be (de)serializable eg. via "NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE" and have default values.
 */
struct ConfigurationData
{
    AppSpecification    app         = {};

    // Vulkan RHI Options
    bool                enableDebugFeatures = true;

    // File paths
    std::string         baseDirPath         = "../Resources";
    std::string         scenesDirName       = "Scenes";
    std::string         fontsDirName        = "Fonts";
    std::string         shadersDirName      = "Shaders";
    std::string         shadersBinDirName   = "Shaders/bin";
    std::string         texturesDirName     = "Textures";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConfigurationData, app);

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
    static std::filesystem::path getSceneFilePath(const std::string& sceneFile) noexcept;

    static std::filesystem::path getFontFilePath(const std::string& fontFile) noexcept;

    static std::filesystem::path getShaderFilePath(const std::string& shaderFile) noexcept;

    static std::filesystem::path getShaderSourceFilePath(const std::string& shaderFile) noexcept;

    static std::filesystem::path getTextureFilePath(const std::string& textureFile) noexcept;

private:
    Configuration();

    void writeConfigFile() const;

    void readConfigFile();

    static UPtr<Configuration> sInstance;

    ConfigurationData mData;
};
