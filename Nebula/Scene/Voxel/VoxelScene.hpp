#pragma once

#include "GPUVoxelInstanceData.hpp"
#include "TerrainGenerator.hpp"
#include "vxlRenderPath.hpp"
#include "Features/FoliageGenerator.hpp"
#include "Features/PillarGenerator.hpp"
#include "Math/Transform.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Geometry/Geometry.hpp"
#include "VulkanRHI/Barrier.hpp"

struct VoxelSceneParams
{
    glm::mat4 globalScale = Transform().setScale(glm::vec3(0.1f)).getModel();
};

class VoxelScene : public Scene
{
public:
    explicit VoxelScene(const SceneCreateInfo& createInfo);

    void render(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept override;

private:
    void generateTerrainVoxelData() noexcept;

    void createForwardPass() noexcept;

    friend class vxlRenderPath;

    UPtr<vxlRenderPath>                         mRenderPath;

    Geometry*                                   mCube;
    SPtr<RHI::Buffer>                           mVertexBuffer;
    SPtr<RHI::Buffer>                           mIndexBuffer;

    std::vector<GPUVoxelInstanceData>           mInstanceData;
    SPtr<RHI::Buffer>                           mInstanceBuffer;

    VoxelSceneParams                            mParams;

    // Forward Pass
    SPtr<RHI::Image>                            mDepthImage;
    SPtr<RHI::GraphicsPipeline>                 mPipeline;
    SPtr<RHI::RenderPass>                       mRenderPass;
};
