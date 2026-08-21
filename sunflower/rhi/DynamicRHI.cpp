#include "DynamicRHI.hpp"

namespace sunflower::rhi
{
    std::uint32_t getMipLevels(const TextureCreateInfo& createInfo, const bool isMultisampled)
    {
        const auto& [size, mipLevels, format, usage, type, label] = createInfo;
        if (type != TextureType::e2D or isMultisampled)
        {
            // Validation warning message: mipLevels == 1u
            if constexpr (conf::gIsDebug)
            {
                if (mipLevels != 1u)
                {
                    spdlog::debug("Non-2D or multisampled must have a mip level count of 1.");
                }
            }
            return 1u;
        }
        const auto maxMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;

        // Validation warning message: mipLevels > maxMipLevels
        if constexpr (conf::gIsDebug)
        {
            if (mipLevels > maxMipLevels)
            {
                spdlog::debug("The specified mip levels ({}) exceed what's supported by the size ({}). [label={}]",
                    mipLevels, maxMipLevels, label.value_or("Unknown"));
            }
        }

        return mipLevels == TextureCreateInfo::sMipLevelsMax
            ? maxMipLevels
            : std::min(mipLevels, maxMipLevels);
    }

    std::uint32_t getTextureLayerCount(const TextureCreateInfo& createInfo) noexcept
    {
        if (createInfo.type == TextureType::eCube)
        {
            return 6u;
        }
        return 1u;
    }

    Size validateTextureSize(const Size& size, const TextureType textureType)
    {
        Size result = {
            .width  = std::max(size.width, 1u),
            .height = std::max(size.height, 1u),
            .depth  = std::max(size.depth, 1u),
        };

        using enum TextureType;
        if (textureType == e1D and (size.height != 1u or size.depth != 1u))
        {
            if constexpr (conf::gIsDebug)
            {
                spdlog::debug("1D Textures must have a height and depth of 1.");
            }
            result.height = result.depth = 1u;
        }
        if ((textureType == e2D or textureType == eCube) and size.depth != 1u)
        {
            if constexpr (conf::gIsDebug)
            {
                spdlog::debug("2D or CubeMap Textures must have a depth of 1.");
            }
            result.depth = 1u;
        }

        return result;
    }
}
