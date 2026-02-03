#include "MoleculeScene.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/color.h>
#include "MoleculeSceneParamsComponent.hpp"
#include "Core/Ranges.hpp"
#include "Scene/Camera/FlyingCamera.hpp"
#include "Scene/Geometry/Geometry.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

MoleculeScene::MoleculeScene(const SceneCreateInfo& createInfo)
: Scene(createInfo)
{
    findAndLoadMolecules();

    const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
    auto camera = makeUnique<FlyingCamera>(glm::ivec2(width, height), glm::vec3(0.0f, 0.0f, 5.0f));
    addCamera(std::move(camera), true);

    // Even if the scene is empty, we create a camera before returning.
    if (mMolecules.empty())
    {
        return;
    }

    mActiveMolecule = &mMolecules[0];

    // Molecule : RenderPasses
    // TODO: Get rid of this pipeline duplication
    for (auto& molecule : mMolecules)
    {
        molecule.mSDFComputePass  = makeUnique<Molecule::SDFComputePass>(mRHI, molecule.mCIFData->getAtomPositions());
        molecule.mStructurePass   = makeUnique<Molecule::StructurePass>(mRHI, mSceneDescriptor, molecule.mCIFData.get());
        molecule.mSDFRaymarchPass = makeUnique<Molecule::SDFRaymarchPass>(mRHI, mSceneDescriptor, molecule.mSDFComputePass->getSDFTexture3D());

        molecule.mSDFRaymarchPass->setParams({
            .bboxMin = molecule.mSDFComputePass->getPushConstants().bboxMin,
            .bboxMax = molecule.mSDFComputePass->getPushConstants().bboxMax,
            .sesColor = glm::vec4(36.0f / 255.0f, 26.0f / 255.0f, 97.0f / 255.0f, 1.0f),
            .voxelSize = 0.5f,
            .blending = 0.5f,
            .ls = 1.0f,
            .useSubsurfaceScattering = 1,
            .rayMarchingSteps = 256
        });
    }
}

void MoleculeScene::render(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept
{
    if (!mMoleculeRenderingOptions.hasCalculatedSDF || mMoleculeRenderingOptions.shouldRecalculateSDF)
    {
        mActiveMolecule->mSDFComputePass->execute(commandList, frameData);
        mMoleculeRenderingOptions.shouldRecalculateSDF = false;
        mMoleculeRenderingOptions.hasCalculatedSDF = true;
    }

    mActiveMolecule->mStructurePass->execute(commandList, frameData);
    mActiveMolecule->mSDFRaymarchPass->execute(commandList, frameData);
}

void MoleculeScene::registerUIComponent(MoleculeScene* pMoleculeScene, UserInterface* pUserInterface) noexcept
{
    pUserInterface->addComponent<MoleculeSceneParamsComponent>(pMoleculeScene);
}

void MoleculeScene::findAndLoadMolecules() noexcept
{
    const auto& moleculesDir = Configuration::getConfig().scenes.moleculesDir;
    if (!std::filesystem::exists(moleculesDir))
    {
        spdlog::warn("The directory containing molecule files ({}) does not exist, no molecules were loaded.",
            styled(moleculesDir, fg(fmt::color::red) | fmt::emphasis::italic));
        return;
    }

    std::vector<std::string> fmtLoaded;
    const auto startTime = std::chrono::high_resolution_clock::now();
    for (const auto& file : std::filesystem::directory_iterator(moleculesDir))
    {
        if (file.is_regular_file() && file.path().extension() == ".cif")
        {
            mMolecules.push_back({
                .mCIFData = CIFData::create({
                    .filename       = file.path().string(),
                    .centerMolecule = true,
                    .rhi            = mRHI,
                })
            });
            mLoadedMolecules.push_back(mMolecules.back().mCIFData->getName());
            fmtLoaded.push_back(fmt::format("{}", styled(mMolecules.back().mCIFData->getName(), fg(fmt::color::cyan) | fmt::emphasis::bold)));
        }
    }
    const auto endTime = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> delta = endTime - startTime;
    const float dt = delta.count();

    spdlog::info("Loaded molecule(s): {} (time: {}s)", join(fmtLoaded), dt);
}

void MoleculeScene::changeActiveMolecule(const std::string& name) noexcept
{
    mRHI->getDevice()->waitIdle();

    const auto it = std::ranges::find_if(mMolecules, [&name](const auto& data) -> bool {
        return data.mCIFData->getName() == name;
    });
    if (mMolecules.end() == it)
    {
        spdlog::error("No molecule by the name [{}] is loaded.", name);
        return;
    }

    mActiveMolecule = &mMolecules[std::distance(mMolecules.begin(), it)];
    mMoleculeRenderingOptions.shouldRecalculateSDF = true;
    mMoleculeRenderingOptions.hasCalculatedSDF = false;

    spdlog::info("Active molecule changed to {}", styled(name, fg(fmt::color::cyan) | fmt::emphasis::bold));
}
