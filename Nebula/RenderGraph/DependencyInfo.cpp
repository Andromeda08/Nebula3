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

    void to_json(nlohmann::json& json, const Extent3D& extent)
    {
        json = nlohmann::json {
            { "extent", std::array{extent.width, extent.height, extent.height} }
        };
    }

    void from_json(const nlohmann::json& json, Extent3D& extent)
    {
        extent.setWidth(json.at("extent").at(0)).setHeight(json.at("extent").at(1)).setDepth(json.at("extent").at(2));
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

        if (auto* pImage3DInfo = std::get_if<Image3DInfo>(&dependencyInfo.resourceParams))
        {
            nlohmann::json imageInfo = {
                { "sType", ImageInfo::sType },
                { "imageUsage", pImage3DInfo->imageUsage }
            };
            if (pImage3DInfo->format != ImageInfo::sDefaultFormat)
            {
                imageInfo["format"] = pImage3DInfo->format;
            }
            imageInfo["extent"] = pImage3DInfo->extent;

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
            if (params.at("sType").get<std::string>() == std::string(Image3DInfo::sType))
            {
                Image3DInfo image3DInfo = {
                    .imageUsage = params.at("imageUsage"),
                    .format     = params.value("format", ImageInfo::sDefaultFormat),
                    .extent     = params.at("extent"),
                };

                dependencyInfo.resourceParams = image3DInfo;
            }
        }
    }
}
