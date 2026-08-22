#pragma once

#include "Common.hpp"

/**
 *
 */
namespace sunflower::rhi
{
    enum class Format : std::uint16_t
    {
        None,
        RGBA8_Unorm,
        RGBA8_Srgb,
        BGRA8_Unorm,
        BGRA8_Srgb,
        R32_Float,
        RG32_Float,
        RGBA16_Float,
        RGBA32_Float,
        D32_Float,
    };

    enum class TextureUsage : std::uint32_t
    {
        None        = 0,
        Sampled     = 1u << 0u,
        Storage     = 1u << 1u,
        ColorTarget = 1u << 2u,
        DepthTarget = 1u << 3u,
        TransferSrc = 1u << 4u,
        TransferDst = 1u << 5u,
    };
    sunflower_BasicFlags(TextureUsage);

    enum class TextureDescriptorType : std::uint8_t
    {
        Sampled,
        Storage,
    };

    enum class TextureType : std::uint8_t
    {
        e1D,
        e2D,
        e3D,
        eCube,
    };

    struct TextureCreateInfo
    {
        // Computes the number of mip levels for the specified size.
        static constexpr auto sMipLevelsMax = 0u;

        Size            size      = {};
        std::uint32_t   mipLevels = 1;
        Format          format    = Format::RGBA8_Unorm;
        TextureUsage    usage     = TextureUsage::Sampled;
        TextureType     type      = TextureType::e2D;
        Option<String>  label     = std::nullopt;
    };

    struct Texture
    {
        sunflower_INTERFACE(Texture);

        virtual Format getFormat() const noexcept = 0;

        virtual const Size& getSize() const noexcept = 0;
    };

    /**
     * Gets the mip levels based on the Texture specifications.
     * @note Validates the following in debug mode:
     * - Mip levels of non-2D and multisampled images must equal 1.
     * - The user specified mip levels can't exceed the levels that the given size can accommodate.
     */
    [[nodiscard]] std::uint32_t getMipLevels(const TextureCreateInfo& createInfo, bool isMultisampled = false);

    [[nodiscard]] std::uint32_t getTextureLayerCount(const TextureCreateInfo& createInfo) noexcept;

    [[nodiscard]] Size validateTextureSize(const Size& size, TextureType textureType);

    inline constexpr auto gSwapchainFormat = Format::BGRA8_Unorm;

    struct Swapchain
    {
        sunflower_INTERFACE(Swapchain);

        virtual const SPtr<Texture>& getTexture(uint64_t i) const = 0;

        virtual Format getFormat() const noexcept = 0;

        virtual Size getSize() const noexcept = 0;
    };

    struct DynamicRHI
    {
        sunflower_INTERFACE(DynamicRHI);

        virtual SPtr<Texture> createTexture(const TextureCreateInfo& textureInfo) = 0;
    };
}
