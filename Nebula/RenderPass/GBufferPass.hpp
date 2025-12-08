#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/IPass.hpp"
#include "VulkanRHI/Rendering.hpp"

namespace rg
{
    class SceneDataResource;
}

namespace RHI
{
    class VulkanRHI;
}

struct GBufferPassCreateInfo
{
    SPtr<RHI::VulkanRHI> rhi;
};

class GBufferPass final : public IPass
{
public:
    nbl_DISABLE_COPY(GBufferPass);
    nbl_CTOR(GBufferPass);

    ~GBufferPass() override = default;

    void initialize() override;

    void execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) override;

    void setResource(const std::string& name, const SPtr<rg::Resource>& resource) override;

    rg::Resource* getResource(const std::string& name) override;

    static rg::NodeCreateInfo getNodeInfo() noexcept;

private:
    static constexpr auto sName = "G-Buffer Pass";
    static constexpr auto sType = rg::NodeType::GBufferPass;

    rg::SceneDataResource*                              mScene = nullptr;

    UPtr<RHI::RenderPass>                               mRenderPass;
    UPtr<RHI::GraphicsPipeline>                         mPipeline;
    std::unordered_map<std::string, SPtr<rg::Resource>> mResources;
    SPtr<RHI::VulkanRHI>                                mRHI;
};