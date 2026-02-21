#include "VoxelScene.hpp"

#include "Scene/Camera/FlyingCamera.hpp"

VoxelScene::VoxelScene(const SceneCreateInfo& createInfo)
: Scene(createInfo)
{
    mCube = addGeometry<Cube>(Cube::Params{});

    const auto vertexSize = mCube->vertexCount() * sizeof(Vertex);
    mVertexBuffer = mRHI->createBuffer({
        .size  = vertexSize,
        .type  = RHI::BufferType::Vertex,
        .label = "Voxel-VertexBuffer",
    });
    mRHI->immediate_uploadToBuffer(mVertexBuffer.get(), mCube->getVertices().data(), vertexSize);

    const auto indexSize = mCube->indexCount() * sizeof(uint32_t);
    mIndexBuffer = mRHI->createBuffer({
        .size  = indexSize,
        .type  = RHI::BufferType::Index,
        .label = "Voxel-IndexBuffer",
    });
    mRHI->immediate_uploadToBuffer(mIndexBuffer.get(), mCube->getIndices().data(), indexSize);

    const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
    auto camera = makeUnique<FlyingCamera>(glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));
    addCamera(std::move(camera), true);

    mLights->addLight({});

    generateTerrainVoxelData();

    mRenderPath = makeUnique<vxlRenderPath>(mRHI, this);
}

void VoxelScene::render(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept
{
    mRenderPath->execute(commandList, frameData);
}

void VoxelScene::generateTerrainVoxelData() noexcept
{
    auto terrainGenerator = vxl::TerrainGenerator({ 256, 24, 96, true });
    terrainGenerator.addGenerator<vxl::FoliageGenerator>(vxl::FoliageGenerator::Control{
        .patchCount             = 12,
        .patchRadius            = 12,
        .radiusVariance         = 3,
        .density                = 0.65f,
        .patchDensityVariance   = true,
        .instanceRandomOffset   = true,
        .instanceRandomScale    = true,
    });

    terrainGenerator.generate();

    mInstanceData = terrainGenerator.getResult()
        | std::views::transform([](const vxl::VoxelData& data) -> GPUVoxelInstanceData {
            auto t = Transform().setScale(data.scale).setTranslate(data.position);
            return {
                .model = t.getModel(),
                .color = glm::vec4(data.color, 1.0f),
            };
        })
        | std::ranges::to<std::vector>();
    spdlog::debug("Generated {} voxels", mInstanceData.size());

    const auto size = mInstanceData.size() * sizeof(GPUVoxelInstanceData);
    mInstanceBuffer = mRHI->createBuffer({
        .size  = size,
        .type  = RHI::BufferType::Vertex,
        .label = "Voxel-InstanceData",
    });
    mRHI->immediate_uploadToBuffer(mInstanceBuffer.get(), mInstanceData.data(), size);
}
