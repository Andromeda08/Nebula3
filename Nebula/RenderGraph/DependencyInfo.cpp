#include "DependencyInfo.hpp"

#include <vulkan/vulkan.hpp>
#include "RGConfiguration.hpp"

namespace vk
{
    void to_json(nlohmann::json& json, const Extent2D& extent)
    {
        json = nlohmann::json {
            { "extent", std::array{extent.width, extent.height} }
        };
    }

    void from_json(const nlohmann::json& json, Extent2D& extent)
    {
        extent.setWidth(json.at("extent").at(0)).setHeight(json.at("extent").at(1));
    }
}

namespace rg
{
    void to_json(nlohmann::json& json, const DependencyInfo& dependencyInfo)
    {
        json = {
            { "id", dependencyInfo.id },
            { "name", dependencyInfo.name },
            { "dependencyType", dependencyInfo.dependencyType },
            { "resourceType", dependencyInfo.resourceType },
            { "dontOptimize", dependencyInfo.dontOptimize },
        };

        if (auto* pImageInfo = std::get_if<ImageInfo>(&dependencyInfo.resourceParams))
        {
            nlohmann::json imageInfo = {
                { "sType", ImageInfo::sType },
                { "imageUsage", pImageInfo->imageUsage }
            };
            if (pImageInfo->format != ImageInfo::sDefaultFormat)
            {
                imageInfo["format"] = pImageInfo->format;
            }
            if (pImageInfo->extent.has_value())
            {
                imageInfo["extent"] = pImageInfo->extent.value();
            }

            json["resourceParams"] = imageInfo;
        }
    }

    void from_json(const nlohmann::json& json, DependencyInfo& dependencyInfo)
    {
        dependencyInfo.id = json.at("id");
        dependencyInfo.name = json.at("name");
        dependencyInfo.dependencyType = json.at("dependencyType");
        dependencyInfo.resourceType = json.at("resourceType");
        dependencyInfo.dontOptimize = json.at("dontOptimize");
        dependencyInfo.resourceParams = std::monostate {};

        if (json.contains("resourceParams"))
        {
            const auto& params = json.at("resourceParams");
            if (params.at("sType").get<std::string>() == std::string(ImageInfo::sType))
            {
                ImageInfo imageInfo = {
                    .imageUsage = params.at("imageUsage"),
                    .format     = params.value("format", ImageInfo::sDefaultFormat),
                };
                if (params.contains("extent"))
                {
                    imageInfo.extent = params.at("extent");
                }

                dependencyInfo.resourceParams = imageInfo;
            }
        }
    }
}
