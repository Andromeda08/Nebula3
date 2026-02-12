#pragma once

#include <metal/metal.hpp>
#include "RHI/RHI.hpp"

namespace RHI
{
    struct MetalTextureState
    {

    };

    class MetalTexture : public ITexture
    {
    public:
        nbl_DISABLE_COPY(MetalTexture);

        MetalTexture(const TextureCreateInfo& createInfo, const NSPtr<MTL::Device>& device);

        [[nodiscard]] static SPtr<MetalTexture> create(const TextureCreateInfo& createInfo, const NSPtr<MTL::Device>& device) noexcept;

        ~MetalTexture() override = default;

        [[nodiscard]] MTL::Texture* getHandle() const
        {
            return mTexture.get();
        }

        void updateTrackedState(TextureUsage dstUsage) noexcept override;

        [[nodiscard]] TextureUsage getCurrentState() const noexcept override;

        [[nodiscard]] const TextureProperties& getProperties() const noexcept override;

    private:
        TextureUsage        mCurrentUsage;
        MetalTextureState   mState;

        TextureProperties   mProperties;

        NSPtr<MTL::Device>  mDevice;
        NSPtr<MTL::Texture> mTexture;
    };
}
