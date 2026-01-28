#pragma once

#include "RenderGraph/RenderPath/Resource.hpp"

namespace rg
{
    class Texture3DResource : public Resource
    {
    public:
        nbl_DISABLE_COPY(Texture3DResource);

        ~Texture3DResource() override = default;

        Texture3DResource(const SPtr<RHI::Texture>& texture, const std::string& name)
        : Resource(name, ResourceType::Texture3D)
        , mTexture(texture)
        {
        }

        [[nodiscard]] const SPtr<RHI::Texture>& getTexture() const noexcept
        {
            return mTexture;
        }

    private:
        SPtr<RHI::Texture>  mTexture;
    };
}
