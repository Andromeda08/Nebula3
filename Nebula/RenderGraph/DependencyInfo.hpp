#pragma once

#include <string>
#include <nlohmann/json.hpp>

#include "RenderGraphTraits.hpp"
#include "RGConfiguration.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace RHI
{
    NLOHMANN_JSON_SERIALIZE_ENUM(ImageUsage, {
        { ImageUsage::Undefined,        "Undefined"         },
        { ImageUsage::ColorAttachment,  "ColorAttachment"   },
        { ImageUsage::Clear,            "Clear"             },
        { ImageUsage::General,          "General"           },
        { ImageUsage::ShaderReadOnly,   "ShaderReadOnly"    },
        { ImageUsage::StorageImage,     "StorageImage"      },
        { ImageUsage::TransferSrc,      "TransferSrc"       },
        { ImageUsage::TransferDst,      "TransferDst"       },
        { ImageUsage::PresentSrc,       "PresentSrc"        },
    });
}

namespace rg
{
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

        // ❗If resourceType is Image
        RHI::ImageUsage     imageUsage      = RHI::ImageUsage::Undefined;

        // Meta data
        uint64_t            requiredMemory  = 0;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DependencyInfo, id, name, dependencyType, resourceType, dontOptimize, imageUsage, requiredMemory);
}
