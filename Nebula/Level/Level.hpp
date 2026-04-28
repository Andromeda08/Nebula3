#pragma once

#include "CullStats.hpp"
#include "Core/Types.hpp"
#include "Geometry/GeometrySystem.hpp"
#include "Instance/InstanceSystem.hpp"
#include "Level/Camera/CameraSystem.hpp"
#include "Level/Light/LightSystem.hpp"
#include "Level/Object/Object.hpp"
#include "Material/MaterialPool.hpp"
#include "Object/SelectObjectFeature.hpp"
#include "Raytracing/BLASSystem.hpp"
#include "Raytracing/TLASSystem.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

class UserInterface;
class TextureManager;

namespace nbl
{
    class Level
    {
    public:
        explicit Level(const SPtr<RHI::VulkanRHI>& rhi, UserInterface* pUserInterface, TextureManager* pTextureManager);

        void onEvent(const SDL_Event& event) noexcept;

        void onUpdate(float dt, const RHI::FrameData& frameData, const RHI::CommandList* pCommandList) noexcept;

        void drawIndexedIndirect(const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept;

        template <class T = Object>
        requires std::derived_from<T, Object>
        T* addObject(
            const int32_t       geometryIndex,
            const Transform&    transform,
            const Handle&       hMaterial,
            const std::string&  name,
            Object*             pParent = nullptr)
        {
            mObjects.push_back(makeUnique<T>());
            auto* obj = static_cast<T*>(mObjects.back().get());

            // Setup Object
            obj->id              = static_cast<int32_t>(mObjects.size()) - 1;
            obj->name            = name.empty() ? fmt::format("Object #{}", mObjects.size()) : name;
            obj->pParent         = pParent;
            obj->geometryIndex   = geometryIndex;
            obj->transform       = transform;
            obj->hMaterial       = hMaterial;
            obj->blasAddress     = 0;
            obj->isInstanceDirty = true;
            obj->hInstance       = mInstanceSystem->acquire({});

            return obj;
        }

        [[nodiscard]] const std::vector<UPtr<Object>>& getObjects() const noexcept
        {
            return mObjects;
        }

        [[nodiscard]] Object* getSelectedObject() const noexcept
        {
            const auto idx = *mSelectObjectFeature->getSelectedObjectIdx();
            if (idx == -1)
            {
                return nullptr;
            }
            return mObjects[idx].get();
        }

        [[nodiscard]] const SPtr<RHI::Buffer>& getInstanceIndirectionBuffer(const uint32_t frameIndex)
        {
            return mInstanceIndirectionMapBuffer[frameIndex];
        }

    private:
        void buildDrawCommands(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData);

        friend class LevelRenderer;
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
    };
}
