#pragma once

#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

#include "InstancePool.hpp"
#include "LightSystem.hpp"
#include "SceneGeometry.hpp"
#include "TextureManager.hpp"
#include "TLASManager.hpp"
#include "Camera/FlyingCamera.hpp"
#include "Components/CullStats.hpp"
#include "Core/Random.hpp"
#include "Core/Ranges.hpp"
#include "Core/Types.hpp"
#include "DataV2/MaterialPool.hpp"
#include "Geometry/Geometry.hpp"
#include "Math/BoundingBox.hpp"
#include "Math/DeltaTime.hpp"
#include "Math/Transform.hpp"
#include "Render/AABBOverlayPass.hpp"
#include "Render/BloomPass.hpp"
#include "Render/FullRT.hpp"
#include "Render/FXAAPass.hpp"
#include "Render/LightingPass.hpp"
#include "Render/ProceduralSky.hpp"
#include "Render/RTAOPass.hpp"

#include "Render/SSAOPass.hpp"
#include "Render/TonemapPass.hpp"
#include "Scene/Render/Indirect_GBufferPass.hpp"
#include "UserInterface/UserInterface.hpp"
#include "Voxel/TerrainGenerator.hpp"
#include "Voxel/Features/FoliageGenerator.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace nbl
{
    class GBufferPass;
}

enum SceneBindings : int32_t
{
    SceneBindings_Camera      = 0,
    SceneBindings_Lights      = 1,
    SceneBindings_Materials   = 2,
    SceneBindings_TopLevelAS  = 3,
};

struct Object
{
    virtual      ~Object() = default;
    virtual void onUpdate(float dt) noexcept {}

    // Properties
    int32_t          geometryIndex    = -1;
    int32_t          instanceIndex    = -1;
    Handle           hMaterial        = {};
    Transform        transform        = {};
    nbl::BoundingBox boundingBox      = {};

    // Raytracing Properties
    uint64_t         blasAddress  = 0;
    uint32_t         rt_mask      = 0xff;

    // General
    int32_t          id;
    std::string      name;

    [[nodiscard]] GPUInstanceData getInstanceData(const MaterialPool* pMaterialPool) noexcept
    {
        return {
            .model          = transform.getModel(),
            .min            = glm::vec4(boundingBox.getMin(), 1.0f),
            .max            = glm::vec4(boundingBox.getMax(), 1.0f),
            .blasAddress    = blasAddress,
            .materialIndex  = hMaterial.isNull() ? -1 : static_cast<int32_t>(pMaterialPool->getGpuIndex(hMaterial)),
            .geometryIndex  = geometryIndex,
        };
    }
};

struct ExampleObject : public Object
{
    ~ExampleObject() override = default;

    void onUpdate(const float dt) noexcept override
    {
        transform.rotate(glm::vec3(mRotationFactor) * dt);
    }

private:
    const float mRotationFactor = Random::get(2.5f, 15.0f) * (Random::get<int32_t>() % 2 == 0 ? 1.0f : -1.0f);
};

class SceneV2
{
public:
    explicit SceneV2(const SPtr<RHI::VulkanRHI>& rhi, UserInterface* pUI);

    void preFrame() noexcept
    {
        mLightSystem->upload();
    }

    void onEvent(const SDL_Event& event) const noexcept
    {
        if (mCamera)
        {
            mCamera->onEvent(event);
        }
    }

    void onUpdate(float dt, const RHI::FrameData& frameData, const RHI::CommandList* pCommandList) noexcept;

    void onRender(const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept;

    const std::vector<UPtr<Object>>& getObjects() const noexcept { return mObjects; }

    template <class T>
    requires std::is_base_of_v<Object, T>
    void addObject(
        const GeometryIndex     geometryIndex,
        const Transform&        transform,
        const Handle            hMaterial,
        const std::string&      name
    ) noexcept
    {
        auto obj = makeUnique<T>();
        obj->name          = name;

        // Set object transform and compute initial transformed AABB
        obj->transform     = transform;
        obj->boundingBox   = mGeometry->getGeometry(geometryIndex)->getBoundingBox().getTransformed(obj->transform.getModel());

        // Ray Tracing
        obj->blasAddress   = mRHI->getRaytracingSupport() ? mGeometry->getBlasAddress(geometryIndex) : 0,

        // Set handles and indices
        obj->geometryIndex = geometryIndex;
        obj->hMaterial     = hMaterial;
        obj->instanceIndex = mInstancePool->acquire(obj->getInstanceData(mMaterialPool.get()));

        mObjects.push_back(std::move(obj));

        mObjectCountChanged = true;
    }

    [[nodiscard]] const SPtr<RHI::Descriptor>& getSceneDescriptor() const noexcept
    {
        return mSceneDescriptor;
    }

private:
    void buildDrawCommands(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept;

    friend class Indirect_GBufferPass;
    friend class nbl::GBufferPass;
    friend class SceneInfoComponent;

    SPtr<RHI::VulkanRHI>                mRHI;
    UserInterface*                      mUserInterface;

    // Vertex, Index, BLAS and GeometryInfo buffers
    // Referenced by (via Index):
    // - Objects
    // - InstancePool data
    // ============================================================
    UPtr<SceneGeometry>                 mGeometry;
    UPtr<InstancePool>                  mInstancePool;

    UPtr<TLASManager>                   mTLASManager;

    UPtr<TextureManager>                mTextureManager;

    UPtr<LightSystem>                   mLightSystem;

    UPtr<MaterialPool>                  mMaterialPool;

    UPtr<ICamera>                       mCamera;
    PerFrameArray<SPtr<RHI::Buffer>>    mCameraUniformBuffers;

    SPtr<RHI::Descriptor>               mSceneDescriptor;

    std::vector<UPtr<Object>>           mObjects;
    bool                                mObjectCountChanged = false;

    PerFrameArray<SPtr<RHI::Buffer>>    mDrawStaging;
    PerFrameArray<SPtr<RHI::Buffer>>    mDrawCmdBuffer;
    PerFrameArray<SPtr<RHI::Buffer>>    mInstanceMapBuffer;
    uint32_t                            mDrawCount = 0;
    bool                                mEnableCulling = true;
    bool                                mVisualizeAABBs = false;
    CullStats                           mLastCull = {};

    UPtr<Indirect_GBufferPass>          mGBufferPass;
    UPtr<SSAOPass>                      mSSAO;
    UPtr<RTAOPass>                      mRTAO;
    UPtr<ProceduralSkyPass>             mProcSky;
    UPtr<LightingPass>                  mLightingPass;
    UPtr<BloomPass>                     mBloomPass;
    UPtr<TonemapPass>                   mTonemapPass;
    UPtr<FXAAPass>                      mFXAA;
    UPtr<AABBOverlayPass>               mAABBPass;

    UPtr<FullRTPass>                    mRTPass;

    std::string                         mName = "Scene V2 Test";
};
