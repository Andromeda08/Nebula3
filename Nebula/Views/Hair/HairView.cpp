#include "HairView.hpp"

#include <imgui.h>

#include "Hair/CyLoader.hpp"
#include "Hair/Hybrid/Shared.hpp"
#include "Level/Camera/FlyingCamera.hpp"
#include "Level/Camera/OrbitCamera.hpp"
#include "Level/GLTF/GLTFLoader.hpp"

namespace nbl
{
    HairView::HairView(nbl_ViewCtorParams)
    : nbl_ViewBaseCtor
    {
        mName = "HairView";

        mCameraSystem = makeUnique<CameraSystem>(mRHI);
        mCameraSystem->addCamera<OrbitCamera>(true, 250.0f);

        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        mCameraSystem->addCamera<FlyingCamera>(false, glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));

        mLightSystem = makeUnique<LightSystem>(mRHI);
        const auto hInitLight = mLightSystem->addLight({
            .vector    = { -2.5f, 6.1f, 3.5f },
            .intensity = 1.0f,
            .type      = LightType::Directional,
        });

        /* Load Hair Models */ {
            mHairModelSystem = makeUnique<HairModelSystem>(mRHI);
            for (const auto& file : std::filesystem::directory_iterator(Configuration::getHairDir()))
            {
                if (file.path().extension() != ".hair")
                {
                    continue;
                }

                try
                {
                    const uint32_t hairIndex = mHairModelSystem->addHairGeometry(nbl::CyLoader(file).load());
                    const auto&    hair      = mHairModelSystem->getHairGeometry(hairIndex);

                    spdlog::info("Loaded Hair model: {} [v={}, S={}, s={}]", hair.name, hair.vertexCount, hair.strandCount, hair.strandletCount);
                }
                catch (const std::runtime_error& ex)
                {
                    spdlog::error(ex.what());
                }
            }

            mHairModelSystem->createBuffers();
        }

        mRenderer = makeUnique<HairRenderer>(mRHI, mHairModelSystem.get());
        mTonemapPass     = TonemapPass::create({
            .outputFormat = HairShared::sColorFormat,
            .rhi = mRHI,
        });

        mGeometrySystem = makeUnique<GeometrySystem>(rhi);
        mMaterialSystem = makeUnique<MaterialSystem>(mRHI, 16);

        GLTFLoader loader({
            .filePath        = Configuration::getSceneFilePath("Model.glb"),
            .pLevel          = nullptr,
            .pTextureManager = mTextureManager,
            .pGeometrySystem = mGeometrySystem.get(),
            .pLightSystem    = mLightSystem.get(),
            .pMaterialSystem = mMaterialSystem.get(),
        });

        loader.load();

        const auto graphicsPS = RHI::GraphicsPS()
            .addDefaultAttachmentState(1)
            .addAttachmentFormat(HairShared::sColorFormat)
            .addAttachmentFormat(HairShared::sDepthFormat);
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("HybridHair_HeadModel")
            .addShader("HybridHair_HeadModel.vert.spv")
            .addShader("HybridHair_HeadModel.frag.spv")
            .setPushConstant<HeadPushConstants>(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

        mHeadPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }

    void HairView::onEvent(const SDL_Event& event)
    {
        mCameraSystem->onEvent(event);
    }

    void HairView::onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        mCameraSystem->onUpdate(frameData);
        mMaterialSystem->onUpdate(pCommandList);
        mGeometrySystem->onUpdate(frameData, pCommandList);
        mLightSystem->onUpdate(pCommandList);
    }

    void HairView::onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        if (mRenderer->mShared->config.renderHead)
        {
            onRender_HeadModel(pCommandList, frameData);
        }

        mRenderer->execute(pCommandList, frameData, {
            .cameraBuffer = mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
            .lightsBuffer = mLightSystem->getBuffer()->getAddress(),
        });

        const auto& tonemapInput = mRenderer->getResult(frameData.currentFrame);
        mTonemapPass->execute(tonemapInput, pCommandList, frameData);

        auto* pFinalImage = mTonemapPass->getResult(frameData.currentFrame).get();
        pCommandList->blitToSwapchain(pFinalImage, mRHI->getSwapchain(), frameData.acquiredIndex);
    }

    void HairView::onDrawUI()
    {
        mLightSystem->onDrawUI();

        ImGui::Begin("Hair Renderer");

        mCameraSystem->onDrawUI();
        mRenderer->drawUI();

        ImGui::End();
    }

    void HairView::onRender_HeadModel(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const
    {
        RHI::Barrier()
            .addBarrier(mRenderer->mShared->colorTarget[frameData.currentFrame]->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mRenderer->mShared->depthBuffer[frameData.currentFrame]->getBarrier(RHI::ImageUsage::DepthAttachment))
            .addBarrier(mGeometrySystem->getBuffers().getVertexBuffer()->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::Vertex))
            .addBarrier(mGeometrySystem->getBuffers().getIndexBuffer()->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::Index))
            .insert(pCommandList);

        RHI::Rendering()
            .setLabel("HeadModel")
            .setRenderArea(mRenderer->mShared->renderResolution)
            .setViewportScissor(pCommandList)
            .addAttachment(mRenderer->mShared->colorTarget[frameData.currentFrame])
            .addAttachment(mRenderer->mShared->depthBuffer[frameData.currentFrame])
            .execute(pCommandList, [&](RHI::CommandList* cmd) -> void
            {
                auto& info = mGeometrySystem->getGeometryInfo(0);

                cmd->bindPipeline(mHeadPipeline.get());

                const auto pushConstants = HeadPushConstants {
                    .vertexBuffer = mGeometrySystem->getBuffers().getVertexBuffer()->getAddress(),
                    .cameraBuffer = mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
                    .lightBuffer  = mLightSystem->getBuffer()->getAddress(),
                    .baseColor    = mRenderer->mShared->config.headColor,
                    .firstIndex   = info.firstIndex,
                    .firstVertex  = info.firstVertex,
                };
                cmd->pushConstants(&pushConstants);
                cmd->getHandle().bindIndexBuffer(mGeometrySystem->getBuffers().getIndexBuffer()->getHandle(), 0, vk::IndexType::eUint32);

                cmd->drawIndexed(info.indexCount, 1, 0, 0, 0);
            });
    }
}
