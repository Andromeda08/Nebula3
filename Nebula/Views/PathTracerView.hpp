#pragma once

#include "Core/View.hpp"
#include "Level/Level.hpp"
#include "Level/Object/ObjectEditorUI.hpp"
#include "Level/Render/LevelRenderer.hpp"
#include "Level/Render/Templates.hpp"
#include "Math/DeltaTime.hpp"

namespace nbl
{
    class [[nodiscard]] DiscretePDF
    {
    public:
        explicit DiscretePDF(const size_t nItems = 0)
        {
            reserve(nItems);
            clear();
        }

        void reserve(const size_t nItems)
        {
            mCDF.reserve(nItems + 1);
        }

        void clear()
        {
            mCDF.clear();
            mCDF.push_back(0.0f);
            mIsNormalized = false;
        }

        void append(const float pdfValue)
        {
            mCDF.push_back(mCDF[mCDF.size() - 1] + pdfValue);
        }

        size_t size() const
        {
            return mCDF.size() - 1;
        }

        float operator[](const size_t entry) const
        {
            return mCDF[entry + 1] - mCDF[entry];
        }

        bool isNormalized() const
        {
            return mIsNormalized;
        }

        float getSum() const
        {
            return mSum;
        }

        float getNormalization() const
        {
            return mNormalization;
        }

        float normalize()
        {
            mSum = mCDF[mCDF.size() - 1];

            if (mSum > 0.0f)
            {
                mNormalization = 1.0f / mSum;
                for (size_t i = 1; i < mCDF.size(); ++i)
                {
                    mCDF[i] *= mNormalization;
                }
                mCDF[mCDF.size() - 1] = 1.0f;
                mIsNormalized = true;
            }
            else
            {
                mNormalization = 0.0f;
            }
            return mSum;
        }

        size_t sample(const float sampleValue) const
        {
            const auto entry = std::ranges::lower_bound(mCDF, sampleValue);
            const auto index = static_cast<size_t>(std::max(static_cast<std::ptrdiff_t>(0), entry - mCDF.begin() - 1));
            return std::min(index, mCDF.size() - 2);
        }

        size_t sample(const float sampleValue, float& pdf) const
        {
            size_t index = sample(sampleValue);
            pdf = operator[](index);
            return index;
        }

        const std::vector<float>& getValues() const { return mCDF; }

    private:
        std::vector<float>  mCDF;
        float               mSum           = 0.0f;
        float               mNormalization = 0.0f;
        bool                mIsNormalized  = false;
    };

    struct GPUDiscretePDF
    {
        float              sum;
        std::vector<float> cdf;
    };

    struct AreaEmitter
    {
        uint32_t  instanceIndex;
        int32_t   geometryIndex;
        uint32_t  cdfOffset;
        uint32_t  triCount;
        float     totalWeight;
        glm::vec3 radiance;
    };

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
                },
                .setCount  = RHI::gFramesInFlight,
                .debugName = "AntiAliasing_Descriptor",
            });

            for (size_t i = 0; i < RHI::gFramesInFlight; i++)
            {
                mCurrentOutput[i]     = makeRenderTarget(mRHI.get(), fmt::format("PT_Output_{}", i), vk::Format::eR32G32B32A32Sfloat);
                mAccumulatedOutput[i] = makeRenderTarget(mRHI.get(), fmt::format("PT_Accumulated_{}", i), vk::Format::eR32G32B32A32Sfloat);
            }

            for (size_t i = 0; i < RHI::gFramesInFlight; i++)
            {
                const auto descriptorWrite = RHI::DescriptorWrite()
                    .writeStorageImage(0, vk::ImageLayout::eGeneral, mCurrentOutput[i])
                    .writeStorageImage(1, vk::ImageLayout::eGeneral, mAccumulatedOutput[i == 0 ? 0 : 1])
                    .writeStorageImage(2, vk::ImageLayout::eGeneral, mAccumulatedOutput[i == 0 ? 1 : 0]);
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
    };
}
