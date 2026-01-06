#pragma once

#include <optional>
#include <string>
#include <variant>
#include <nlohmann/json.hpp>

#include "RenderGraphTraits.hpp"
#include "VulkanRHI/Image.hpp"

namespace RHI
{
    NLOHMANN_JSON_SERIALIZE_ENUM(ImageUsage, {
        { ImageUsage::Undefined,        "Undefined"         },
        { ImageUsage::ColorAttachment,  "ColorAttachment"   },
        { ImageUsage::DepthAttachment,  "DepthAttachment"   },
        { ImageUsage::Clear,            "Clear"             },
        { ImageUsage::General,          "General"           },
        { ImageUsage::ShaderReadOnly,   "ShaderReadOnly"    },
        { ImageUsage::StorageImage,     "StorageImage"      },
        { ImageUsage::TransferSrc,      "TransferSrc"       },
        { ImageUsage::TransferDst,      "TransferDst"       },
        { ImageUsage::PresentSrc,       "PresentSrc"        },
    });
}

namespace vk
{
    struct Extent2D;

    void to_json(nlohmann::json& json, const Extent2D& extent);
    void from_json(const nlohmann::json& json, Extent2D& extent);
}

namespace rg
{
    struct ImageInfo
    {
        constexpr static auto sDefaultFormat = vk::Format::eR32G32B32A32Sfloat;
        constexpr static auto sType          = "ImageInfo";

        RHI::ImageUsage             imageUsage = RHI::ImageUsage::Undefined;
        vk::Format                  format     = sDefaultFormat;

        // ❗Extent is optional, will default to current swapchain extent.
        std::optional<vk::Extent2D> extent     = std::nullopt;
    };

    struct DependencyInfo
    {
        std::int32_t        id              = -1;
        std::string         name            = "Unknown Resource";
        DependencyType      dependencyType  = DependencyType::Ignored;
        ResourceType        resourceType    = ResourceType::Unknown;
        bool                isConnected     = false;
        ResourceStyle       style;

        // Dependency flags
        bool                dontOptimize    = false;

        // Resource Type specific
        std::variant<std::monostate, ImageInfo> resourceParams;
    };

    void to_json(nlohmann::json& json, const DependencyInfo& dependencyInfo);

    void from_json(const nlohmann::json& json, DependencyInfo& dependencyInfo);
}
