#include "MetalTexture.hpp"

namespace RHI
{
    MetalTexture::MetalTexture(const TextureCreateInfo& createInfo, const NSPtr<MTL::Device>& device)
    : ITexture()
    , mCurrentUsage(TextureUsage::Undefined)
    , mDevice(device)
    {
        mProperties = {
            .extent      = createInfo.extent,
            .format      = createInfo.format,
            .aspectFlags = getAspectFlags(createInfo.format),
            .levelCount  = 1,
            .sampleCount = 1u,
            .type        = getTextureType(createInfo.extent),
        };

        auto* textureDescriptor = MTL::TextureDescriptor::alloc();
        textureDescriptor->setUsage(to_mtl(createInfo.usageFlags));
        textureDescriptor->setWidth(createInfo.extent.width);
        textureDescriptor->setHeight(createInfo.extent.height);
        textureDescriptor->setDepth(createInfo.extent.depth);
        textureDescriptor->setTextureType(to_mtl(createInfo.getTextureType()));
        textureDescriptor->setMipmapLevelCount(1);
        textureDescriptor->setSampleCount(1u);
        textureDescriptor->setPixelFormat(to_mtl(createInfo.format));
        textureDescriptor->setArrayLength(1); // if 2D must equal 1
        textureDescriptor->setSwizzle(MTL::TextureSwizzleChannels::Default());
        textureDescriptor->setStorageMode(MTL::StorageModePrivate);

        mTexture = NS::TransferPtr(mDevice->newTexture(textureDescriptor));
        mTexture->setLabel(NS::String::string(createInfo.label.c_str(), NS::UTF8StringEncoding));

        textureDescriptor->release();
    }

    SPtr<MetalTexture> MetalTexture::create(const TextureCreateInfo& createInfo, const NSPtr<MTL::Device>& device) noexcept
    {
        return makeShared<MetalTexture>(createInfo, device);
    }

    void MetalTexture::updateTrackedState(TextureUsage dstUsage) noexcept
    {
        spdlog::warn("MetalTexture::updateTrackedState() is not implemented (no-op)");
    }

    TextureUsage MetalTexture::getCurrentState() const noexcept
    {
        return mCurrentUsage;
    }

    const TextureProperties& MetalTexture::getProperties() const noexcept
    {
        return mProperties;
    }
}
