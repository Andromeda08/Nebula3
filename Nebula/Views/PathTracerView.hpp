#pragma once

#include "Core/View.hpp"
#include "Level/Level.hpp"
#include "Level/Light/AreaEmitter.hpp"
#include "Level/Light/DiscretePDF.hpp"
#include "Level/Render/LevelRenderer.hpp"
#include "Level/Render/Templates.hpp"

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

        void onDrawUI();

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
        uint64_t materials;     // Material.slang BDA
        uint64_t geometryInfos; // Geometry Info BDA
        uint64_t accumulated;   // Current accumulation frame counter
        uint64_t totalFrames;   // Total lifetime frame counter
        uint32_t maxBounces;    // Maximum number of bounces before RR
        uint32_t spp;           // Samples-per-pixel
        int32_t  bDynamicRR;    // Use dynamic rr continuation probability based on throughput?
        float    rrCont;        // RR continuation probability if !bDynamicRR
        uint32_t emitterCount;  // N emitters
        float    jitterX;       // Current frame camera jitter X
        float    jitterY;       // Current frame camera jitter Y
    };

    class PathTracerView : public View
    {
    public:
        PathTracerView(nbl_ViewCtorParams);

        ~PathTracerView() override = default;

        void onEvent(const SDL_Event& event) override;

        void onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override;

        void onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override;

        void onDrawUI() override;

    private:
        void execute_DLSSDenoiser(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const;

        static float halton(uint32_t index, uint32_t base);

        float                           mJitterX     = 0.0f;
        float                           mJitterY     = 0.0f;
        uint64_t                        mTotalFrames = 0;

        UPtr<PTScene>                   mScene;

        SPtr<RHI::Descriptor>           mDescriptor;
        UPtr<RHI::RayTracingPipeline2>  mPipeline;

        // 1 spp output
        PerFrameArray<SPtr<RHI::Image>> mCurrentOutput;     // RGBA32

        // G-Buffer output
        PerFrameArray<SPtr<RHI::Image>> mNormals;           // RGBA16, World-space
        PerFrameArray<SPtr<RHI::Image>> mDiffuseAlbedo;     // RGBA16
        PerFrameArray<SPtr<RHI::Image>> mSpecularAlbedo;    // RGBA16
        PerFrameArray<SPtr<RHI::Image>> mRoughness;         // R16
        PerFrameArray<SPtr<RHI::Image>> mDepth;             // R32
        PerFrameArray<SPtr<RHI::Image>> mMotionVectors;     // RG16
        PerFrameArray<SPtr<RHI::Image>> mSpecularHitDist;   // R32

        PerFrameArray<SPtr<RHI::Image>> mRROutput;          // RGBA16

        // AgX
        UPtr<TonemapPass>               mTonemapPass;
    };
}
