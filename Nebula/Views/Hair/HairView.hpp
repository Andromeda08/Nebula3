#pragma once

#include "Core/View.hpp"
#include "Hair/HairGeometry.hpp"
#include "Hair/Hybrid/HairRenderer.hpp"
#include "Level/Camera/CameraSystem.hpp"
#include "Level/Geometry/GeometrySystem.hpp"
#include "Level/Light/LightSystem.hpp"
#include "Level/Material/MaterialPool.hpp"
#include "Level/Render/TonemapPass.hpp"

namespace nbl
{
    class HairView : public View
    {
        struct HeadPushConstants
        {
            uint64_t       vertexBuffer;
            uint64_t       cameraBuffer;
            uint64_t       lightBuffer;
            glm::vec3      baseColor;
            uint32_t       firstIndex;
            uint32_t       firstVertex;
        };

    public:
        HairView(nbl_ViewCtorParams);

        ~HairView() override = default;

        void onEvent(const SDL_Event& event) override;

        void onUpdate(float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override;

        void onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override;

        void onDrawUI() override;

    private:
        void onRender_HeadModel(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const;

        UPtr<CameraSystem>              mCameraSystem;
        UPtr<LightSystem>               mLightSystem;
        UPtr<HairModelSystem>           mHairModelSystem;

        UPtr<GeometrySystem>            mGeometrySystem;
        UPtr<MaterialSystem>            mMaterialSystem;

        UPtr<HairRenderer>              mRenderer;

        UPtr<RHI::GraphicsPipeline2>    mHeadPipeline;

        UPtr<TonemapPass>               mTonemapPass;
    };
}
