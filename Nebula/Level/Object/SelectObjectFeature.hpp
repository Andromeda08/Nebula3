#pragma once

#include <cstdint>

#include "Core/App.hpp"
#include "Level/Camera/CameraSystem.hpp"
#include "Level/Raytracing/TLASSystem.hpp"
#include "Level/Render/PrePass.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

// SelectObjectFeature.hpp
// Allows for picking objects by using Ray Queries to cast
// a ray into the bound Top Level AS. (requires RT)
// ============================================================

namespace nbl
{
    struct Object;

    class SelectObjectFeature
    {
        struct PushConstant
        {
            uint64_t  instanceAddress;
            uint64_t  cameraAddress;
            uint64_t  selectAddress;
            glm::vec2 mousePos;
            glm::vec2 screenSize;
        };
    public:
        SelectObjectFeature(const SPtr<RHI::VulkanRHI>& rhi, CameraSystem* pCameraSystem, InstanceSystem* pInstanceSystem, TLASSystem* pTlasSystem, const PrePass* pPrePass);

        void onEvent(const SDL_Event& event) noexcept;

        void onDrawUI(const std::vector<UPtr<Object>>& objects);

        [[nodiscard]] int32_t* getSelectedObjectIdx() noexcept;

    private:
        SPtr<RHI::VulkanRHI>        mRHI;
        CameraSystem*               mCameraSystem;
        InstanceSystem*             mInstanceSystem;
        TLASSystem*                 mTlasSystem;

        SPtr<RHI::Descriptor>       mDescriptor;

        int32_t                     mSelectedObject = -1;
        SPtr<RHI::Buffer>           mObjSelectBuffer;
        SPtr<RHI::ComputePipeline2> mObjSelectPipeline;
    };
}