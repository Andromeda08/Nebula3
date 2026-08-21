#include "DynamicRHI.hpp"

namespace sunflower::rhi
{
    std::uint32_t getMipLevels(const TextureCreateInfo& createInfo, const bool isMultisampled) noexcept
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
}
