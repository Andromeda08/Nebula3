#pragma once

#include <rhi/Common.hpp>
#include <rhi/DynamicRHI.hpp>
#include <rhi/vulkan/Device.hpp>
#include <rhi/vulkan/VulkanCommon.hpp>

namespace sunflower::rhi
{
    class VulkanSwapchain;

    struct VulkanSwapchainWrappedTextureCreateInfo
    {
        VulkanSwapchain* pSwapchain;
        vk::Image        handle;
        Option<String>   label;
    };

    class VulkanTexture final : public Texture
    {
    public:
        sunflower_DisableCopy(VulkanTexture);
        // ctor: Regular Texture.
        sunflower_CreateResource(VulkanTexture, TextureCreateInfo,                       Device, SPtr);
        // ctor: Texture for wrapping a Swapchain image.
        sunflower_CreateResource(VulkanTexture, VulkanSwapchainWrappedTextureCreateInfo, Device, SPtr);

        ~VulkanTexture() override;

        [[nodiscard]] Format getFormat() const noexcept override;

        [[nodiscard]] const Size& getSize() const noexcept override;

        // -----------------------------
        // Vulkan Specific
        // -----------------------------

        [[nodiscard]] const vk::Image& getHandle() const noexcept;

        [[nodiscard]] const vk::ImageView& getImageView() const noexcept;

        [[nodiscard]] vk::ImageSubresourceRange getSubresourceRange() const noexcept;

    private:
        // Wrapped image iff mSwapchain is not null.
        [[nodiscard]] bool isWrappedImage() const noexcept;

        void createImage(const TextureCreateInfo& createInfo);

        void createImageView(const Option<String>& label = std::nullopt);

        SPtr<Device>        mDevice     = nullptr;
        VulkanSwapchain*    mSwapchain  = nullptr;

        vk::Image           mImage      = nullptr;
        vk::ImageView       mImageView  = nullptr;

        VmaAllocation       mAlloc      = nullptr;
        VmaAllocationInfo   mAllocInfo  = {};

        const Format        mFormat     = Format::None;
        const Size          mSize       = {};
        const TextureType   mType       = TextureType::e2D;
        const std::uint32_t mMipLevels  = 1u;
        const std::uint32_t mLayers     = 1u;
    };
}
