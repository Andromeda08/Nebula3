#pragma once

#include "CullStats.hpp"
#include "Core/Types.hpp"
#include "Geometry/GeometrySystem.hpp"
#include "Instance/InstanceSystem.hpp"
#include "Level/Camera/CameraSystem.hpp"
#include "Level/Light/LightSystem.hpp"
#include "Level/Object/Object.hpp"
#include "Light/AreaEmitter.hpp"
#include "Light/DiscretePDF.hpp"
#include "Material/MaterialPool.hpp"
#include "Object/SelectObjectFeature.hpp"
#include "Raytracing/BLASSystem.hpp"
#include "Raytracing/TLASSystem.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

class UserInterface;
class TextureManager;

namespace nbl
{
    struct ObjectParams
    {
        int32_t                     geometryIndex   = -1;
        Transform                   transform       = {};
        Handle                      hMaterial       = {};
        std::optional<std::string > name            = std::nullopt;
        Object*                     pParent         = nullptr;

        // Path Tracer Emitters
        bool                        isEmitter       = false;
        std::optional<glm::vec3>    radiance        = std::nullopt;
    };

    struct PendingReleaseLevel
    {
        SPtr<RHI::Buffer> buffer;
        uint64_t          frameToRelease;
    };

    class Level
    {
    public:
        explicit Level(const SPtr<RHI::VulkanRHI>& rhi, UserInterface* pUserInterface, TextureManager* pTextureManager);

        void onEvent(const SDL_Event& event) noexcept;

        void onUpdate(float dt, const RHI::FrameData& frameData, RHI::CommandList* pCommandList) noexcept;

        void drawIndexedIndirect(const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept;

        uint64_t getCameraBuffer(const uint32_t frame) const;

        template <class T = Object>
        requires std::derived_from<T, Object>
        T* addObject(const ObjectParams& objParams)
        {
            mObjects.push_back(makeUnique<T>());
            auto* obj = static_cast<T*>(mObjects.back().get());

            // Setup Object
            obj->id              = static_cast<int32_t>(mObjects.size()) - 1;
            obj->name            = objParams.name.value_or(fmt::format("Object #{}", mObjects.size()));
            obj->pParent         = objParams.pParent;
            obj->geometryIndex   = objParams.geometryIndex;
            obj->transform       = objParams.transform;
            obj->hMaterial       = objParams.hMaterial;
            obj->blasAddress     = 0;
            obj->isInstanceDirty = true;
            obj->hInstance       = mInstanceSystem->acquire({});

            if (objParams.isEmitter)
            {
                const auto* geometry = mGeometrySystem->getGeometry(obj->geometryIndex);

                if (!mDiscretePDFs.contains(obj->geometryIndex))
                {
                    mDiscretePDFs[obj->geometryIndex] = DiscretePDF(mGeometrySystem->getGeometry(obj->geometryIndex));
                }

                const AreaEmitter emitterInfo = {
                    .instanceIndex = mInstanceSystem->getGpuIndex(obj->hInstance),
                    .geometryIndex = obj->geometryIndex,
                    .cdfOffset     = std::numeric_limits<uint32_t>::max(),
                    .triCount      = geometry->getTriangleCount(),
                    .totalWeight   = mDiscretePDFs[obj->geometryIndex].getSum(),
                    .radiance      = objParams.radiance.value_or(mMaterialSystem->get(obj->hMaterial)->solidColor) * 15.0f,
                };

                mEmitters.push_back(emitterInfo);

                obj->emitterIndex = mEmitters.size() - 1;
            }

            return obj;
        }

        [[nodiscard]] const std::vector<UPtr<Object>>& getObjects() const noexcept;

        [[nodiscard]] Object* getSelectedObject() const noexcept;

        [[nodiscard]] const SPtr<RHI::Buffer>& getInstanceIndirectionBuffer(const uint32_t frameIndex);

    private:
        void initEmitterData();

        void buildDrawCommands(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData);

        friend class LevelRenderer;
        friend class LevelPathTracer;
        friend class PrePass;
        friend class GBufferPass;
        friend class LightingPass;
        friend class BoundingBoxDebugPass;

        SPtr<RHI::VulkanRHI>                mRHI;
        UserInterface*                      mUserInterface;
        TextureManager*                     mTextureManager;

        // Systems
        UPtr<CameraSystem>                  mCameraSystem;
        UPtr<GeometrySystem>                mGeometrySystem;
        UPtr<LightSystem>                   mLightSystem;
        UPtr<MaterialSystem>                mMaterialSystem;
        UPtr<InstanceSystem>                mInstanceSystem;

        UPtr<BLASSystem>                    mBlasSystem;
        UPtr<TLASSystem>                    mTlasSystem;

        // Interactive Features
        UPtr<PrePass>                       mPrePass;
        UPtr<SelectObjectFeature>           mSelectObjectFeature;

        // Draw Commands, Culling Stats and Configuration
        PerFrameArray<SPtr<RHI::Buffer>>    mBuildDrawCommandsStaging;
        PerFrameArray<SPtr<RHI::Buffer>>    mDrawCommandsBuffer;
        PerFrameArray<SPtr<RHI::Buffer>>    mInstanceIndirectionMapBuffer;

        uint32_t                            mDrawCount     = 0;
        bool                                mEnableCulling = true;
        CullStats                           mLastCullStats = {};

        // Objects
        std::vector<UPtr<Object>>           mObjects;

        // Geometry Index -> DiscretePDF
        std::unordered_map<int32_t, DiscretePDF> mDiscretePDFs;

        std::vector<AreaEmitter>  mEmitters;
        SPtr<RHI::Buffer>         mEmittersBuffer;
        SPtr<RHI::Buffer>         mDiscretePDFsBuffer;

        std::vector<PendingRelease> mPendingReleases;
    };
}
