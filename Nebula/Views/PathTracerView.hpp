#pragma once

#include "Core/View.hpp"
#include "Level/Level.hpp"
#include "Level/Light/AreaEmitter.hpp"
#include "Level/Light/DiscretePDF.hpp"
#include "Level/Object/ObjectEditorUI.hpp"
#include "Level/Render/LevelRenderer.hpp"
#include "Level/Render/Templates.hpp"
#include "Math/DeltaTime.hpp"

namespace nbl
{
    struct PTObjectParams
    {
        int32_t                    geometryIndex = -1;
        Transform                  transform     = {};
        Handle                     hMaterial     = {};
        bool                       isEmitter     = false;
        std::optional<glm::vec3>   radiance      = std::nullopt;
        std::optional<std::string> name          = std::nullopt;
    };

    class PTScene
    {
    public:
        explicit PTScene(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager);

        Object* addObject(const PTObjectParams& params);

        void onEvent(const SDL_Event& event) const noexcept;

        void onUpdate(float dt, const RHI::FrameData& frameData, const RHI::CommandList* pCommandList) const noexcept;

        void initEmitterData();

    private:
        friend class PathTracerView;

        SPtr<RHI::VulkanRHI>      mRHI;

        UPtr<CameraSystem>        mCameraSystem;
        UPtr<GeometrySystem>      mGeometrySystem;
        UPtr<LightSystem>         mLightSystem;
        UPtr<MaterialSystem>      mMaterialSystem;
        UPtr<InstanceSystem>      mInstanceSystem;
        UPtr<BLASSystem>          mBlasSystem;
        UPtr<TLASSystem>          mTlasSystem;

        UPtr<SelectObjectFeature> mSelectObjectFeature;

        std::vector<UPtr<Object>> mObjects;

        // Geometry Index -> DiscretePDF
        std::unordered_map<int32_t, DiscretePDF> mDiscretePDFs;

        std::vector<AreaEmitter>  mEmitters;
        SPtr<RHI::Buffer>         mEmittersBuffer;
        SPtr<RHI::Buffer>         mDiscretePDFsBuffer;
    };

    struct PathTracerPushConstants
    {
        uint64_t camera;        // Camera BDA
        uint64_t prevCamera;    // Camera BDA
        uint64_t instances;     // Instance BDA
        uint64_t emitters;      // Emitters BDA
        uint64_t emitterPdfs;   // Emitter Discrete PDF BDA
        uint64_t vertices;      // Vertex BDA
        uint64_t indices;       // Index BDA
        uint64_t materials;     // Materials BDA
        uint64_t geometryInfos; // Geometry Info BDA
        uint64_t accumulated;   // Current accumulation frame counter
        uint64_t totalFrames;   // Total lifetime frame counter
        uint32_t maxBounces;    // Maximum number of bounces before RR
        uint32_t spp;           // Samples-per-pixel
        int32_t  bDynamicRR;    // Use dynamic rr continuation probability based on throughput?
        float    rrCont;        // RR continuation probability if !bDynamicRR
        uint32_t emitterCount;  // N emitters
        float    jitterX;
        float    jitterY;
    };

    class PathTracerView : public View
    {
    public:
        PathTracerView(nbl_ViewCtorParams)
        : nbl_ViewBaseCtor
        {
            mName = "PathTracerView";
            mScene = makeUnique<PTScene>(mRHI, mTextureManager);

            mDescriptor = mRHI->createDescriptor({
                .bindings  = {
                    { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                    { 1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                    { 2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                    { 3, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                    { 4, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                    { 5, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                    { 6, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                    { 7, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                    // { 8, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                    // { 9, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                },
                .setCount  = RHI::gFramesInFlight,
                .debugName = "PT_Descriptor",
            });

            for (uint32_t i = 0; i < RHI::gFramesInFlight; i++)
            {
                mNormals[i]         = makeRenderTarget(mRHI.get(), fmt::format("PT_Normals_{}",    i));
                mDiffuseAlbedo[i]   = makeRenderTarget(mRHI.get(), fmt::format("PT_DiffAlbedo_{}", i));
                mSpecularAlbedo[i]  = makeRenderTarget(mRHI.get(), fmt::format("PT_SpecAlbedo_{}", i));
                mRoughness[i]       = makeRenderTarget(mRHI.get(), fmt::format("PT_Roughness_{}",  i), vk::Format::eR16Sfloat);
                mDepth[i]           = makeRenderTarget(mRHI.get(), fmt::format("PT_LinDepth_{}",   i), vk::Format::eR32Sfloat);
                mMotionVectors[i]   = makeRenderTarget(mRHI.get(), fmt::format("PT_MVec_{}",       i), vk::Format::eR32G32Sfloat);
                mRROutput[i]        = makeRenderTarget(mRHI.get(), fmt::format("PT_DLSS_RR_{}",    i));
                mSpecularHitDist[i] = makeRenderTarget(mRHI.get(), fmt::format("PT_SpecHitDist_{}",i), vk::Format::eR32Sfloat);
            }

            for (size_t i = 0; i < RHI::gFramesInFlight; i++)
            {
                mCurrentOutput[i]     = makeRenderTarget(mRHI.get(), fmt::format("PT_Output_{}", i), vk::Format::eR32G32B32A32Sfloat);
                mAccumulatedOutput[i] = makeRenderTarget(mRHI.get(), fmt::format("PT_Accumulated_{}", i), vk::Format::eR32G32B32A32Sfloat);
            }

            for (size_t i = 0; i < RHI::gFramesInFlight; i++)
            {
                const auto descriptorWrite = RHI::DescriptorWrite()
                    .writeStorageImage(0, vk::ImageLayout::eGeneral, mCurrentOutput[i])
                    // .writeStorageImage(1, vk::ImageLayout::eGeneral, mAccumulatedOutput[i == 0 ? 0 : 1])
                    // .writeStorageImage(2, vk::ImageLayout::eGeneral, mAccumulatedOutput[i == 0 ? 1 : 0])
                    .writeStorageImage(1, vk::ImageLayout::eGeneral, mNormals[i])
                    .writeStorageImage(2, vk::ImageLayout::eGeneral, mDiffuseAlbedo[i])
                    .writeStorageImage(3, vk::ImageLayout::eGeneral, mSpecularAlbedo[i])
                    .writeStorageImage(4, vk::ImageLayout::eGeneral, mRoughness[i])
                    .writeStorageImage(5, vk::ImageLayout::eGeneral, mDepth[i])
                    .writeStorageImage(6, vk::ImageLayout::eGeneral, mMotionVectors[i])
                    .writeStorageImage(7, vk::ImageLayout::eGeneral, mSpecularHitDist[i]);
                mDescriptor->write(i, descriptorWrite);
            }

            using enum vk::ShaderStageFlagBits;
            const auto ps     = RHI::RayTracingPS().setMaxDepth(1);
            const auto common = RHI::PipelineCommon()
                .setLabel("PathTracer")
                .addShader("pt.rgen.spv")
                .addShader("pt.miss.spv")
                .addShader("pt.chit.spv")
                .addShader("diffuse.call.spv")
                .addShader("mirror.call.spv")
                .addShader("dielectric.call.spv")
                .addDescriptorLayout(0, mDescriptor.get())
                .addDescriptorLayout(1, mScene->mTlasSystem->getDescriptor().get())
                .setPushConstant<PathTracerPushConstants>(eRaygenKHR | eClosestHitKHR | eMissKHR | eCallableKHR);
            mPipeline = mRHI->createRayTracingPipeline2(ps, common);

            mTonemapPass = makeUnique<TonemapPass>(Tonemap_Params {
                .outputFormat = vk::Format::eR32G32B32A32Sfloat,
                .rhi          = mRHI,
            });

            mFXAAPass = AntiAliasingPass::create({
                .input = mTonemapPass->getResults(),
                .rhi   = mRHI,
            });

            mUserInterface->addComponent<CameraSystemUI>(mScene->mCameraSystem.get());
            mUserInterface->addComponent<ObjectEditorUI>(mScene->mObjects, mScene->mSelectObjectFeature->getSelectedObjectIdx());
        }

        ~PathTracerView() override = default;

        void onEvent(const SDL_Event& event) override;

        void onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override;

        void onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override;

        void onDrawUI() override;

    private:
        static NVSDK_NGX_Resource_VK wrapImage(const SPtr<RHI::Image>& img, const bool readWrite)
        {
            const auto& p = img->getProperties();
            VkImageSubresourceRange range{};
            range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel   = 0;
            range.levelCount     = 1;
            range.baseArrayLayer = 0;
            range.layerCount     = 1;

            return NVSDK_NGX_Create_ImageView_Resource_VK(
                static_cast<VkImageView>(img->getImageView()),
                static_cast<VkImage>(img->getImage()),
                range,
                static_cast<VkFormat>(p.format),
                p.extent.width,
                p.extent.height,
                readWrite);
        }

        static float halton(uint32_t index, uint32_t base)
        {
            float f = 1.0f, r = 0.0f;
            while (index > 0)
            {
                f /= static_cast<float>(base);
                r += f * static_cast<float>(index % base);
                index /= base;
            }
            return r;
        }

        float mJitterX = 0.0f;
        float mJitterY = 0.0f;

        int32_t   mSPP               = 1;
        int32_t   mMaxBounces        = 4;
        bool      mDynamicRR         = false;
        float     mRRCont            = 0.7f;
        int32_t   mMaximumSamples    = 65536;

        uint64_t  mAccumulatedFrames = 0;
        uint64_t  mTotalFrames       = 0;
        glm::mat4 mPrevView          = glm::mat4(1.0f);

        std::chrono::high_resolution_clock::time_point mAccumStartTime = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::time_point mAccumEndTime   = {};

        PerFrameArray<SPtr<RHI::Image>> mCurrentOutput;
        PerFrameArray<SPtr<RHI::Image>> mAccumulatedOutput;

        UPtr<TonemapPass>               mTonemapPass;
        UPtr<AntiAliasingPass>          mFXAAPass;

        UPtr<PTScene>                   mScene;
        SPtr<RHI::Descriptor>           mDescriptor;
        UPtr<RHI::RayTracingPipeline2>  mPipeline;

        // G-Buffer output
        PerFrameArray<SPtr<RHI::Image>> mNormals;           // RGBA16, World-space
        PerFrameArray<SPtr<RHI::Image>> mDiffuseAlbedo;     // RGBA16
        PerFrameArray<SPtr<RHI::Image>> mSpecularAlbedo;    // RGBA16
        PerFrameArray<SPtr<RHI::Image>> mRoughness;         // R16
        PerFrameArray<SPtr<RHI::Image>> mDepth;             // R32
        PerFrameArray<SPtr<RHI::Image>> mMotionVectors;     // RG16
        PerFrameArray<SPtr<RHI::Image>> mSpecularHitDist;   // R32

        PerFrameArray<SPtr<RHI::Image>> mRROutput;          // RGBA16
    };
}
