#pragma once

#include "RenderGraph/RenderPath/Resource.hpp"

namespace rg
{
    class Texture2DResource : public Resource
    {
    public:
        nbl_DISABLE_COPY(Texture2DResource);

        ~Texture2DResource() override = default;

        Texture2DResource(const SPtr<RHI::Texture>& texture, const std::string& name, const bool isAliased)
        : Resource(name, ResourceType::Texture2D)
        , mTexture(texture)
        , mAliased(isAliased)
        {
        }

        [[nodiscard]] SPtr<RHI::Texture> getTexture() const noexcept
        {
            return mTexture;
        }

        void useAliasedAllocation(const SPtr<RHI::Allocation>& pAllocation) const noexcept
        {
            mTexture->useAliasedAllocation(pAllocation);
        }

    private:
        SPtr<RHI::Texture>  mTexture;
        bool                mAliased;
    };
}
