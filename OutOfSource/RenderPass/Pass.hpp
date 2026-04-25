#pragma once

#include "VulkanRHI/Frame.hpp"
#include "VulkanRHI/Commands/CommandList.hpp"

namespace rg
{
    class Resource;
}

namespace res
{
    static constexpr auto rScene = "Scene Data";
    static constexpr auto rPositionBuffer = "Position Buffer";
    static constexpr auto rNormalBuffer = "Normal Buffer";
    static constexpr auto rAlbedoBuffer = "Albedo Buffer";
    static constexpr auto rDepthBuffer = "Depth Buffer";

    // Molecule Rendering
    static constexpr auto rSDFTexture = "SDF Texture";
    static constexpr auto rStructureRender = "Structure Render";
    static constexpr auto rFinalRender = "Final Render";
}

class Pass
{
public:
    virtual ~Pass() = default;

    virtual void initialize() {}
    virtual void update() {}
    virtual void execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) = 0;

    virtual void setResource(const std::string& name, const SPtr<rg::Resource>& resource) noexcept
    {
        mResources.insert_or_assign(name, resource);
    }

    virtual rg::Resource* getResource(const std::string& name) const noexcept
    {
        return mResources.contains(name) ? mResources.at(name).get() : nullptr;
    }

protected:
    std::map<std::string, SPtr<rg::Resource>> mResources;
};
