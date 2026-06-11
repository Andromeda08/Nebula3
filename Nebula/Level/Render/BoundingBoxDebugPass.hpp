#pragma once

#include <glm/glm.hpp>
#include "Core/Types.hpp"
#include "Level/Level.hpp"
#include "UserInterface/UserInterface.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class Level;

    struct BoundingBoxDebugPass_Params
    {
        Level*                          pLevel             = nullptr;
        PerFrameArray<SPtr<RHI::Image>> renderTargets      = { nullptr, nullptr };
        SPtr<RHI::Image>                gBufferDepthBuffer = nullptr;
        SPtr<RHI::VulkanRHI>            rhi                = nullptr;
    };

    /**
     * Debug renderer for BoundingBox.
     * Render the outline of all or the selected object's bounding box.
     */
    class BoundingBoxDebugPass
    {
        struct PushConstants
        {
            glm::vec4   boxColor;
            uint64_t    instanceBuffer;
            uint64_t    cameraBuffer;
            uint32_t    instanceIndex;
        };
    public:
        nbl_DisableCopy(BoundingBoxDebugPass);
        nbl_CreateWithStruct(BoundingBoxDebugPass, BoundingBoxDebugPass_Params);

        explicit BoundingBoxDebugPass(const BoundingBoxDebugPass_Params& params);

        ~BoundingBoxDebugPass() = default;

        void execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;

    private:
        friend class BoundingBoxDebugPassUI;

        void init() noexcept;

        bool hasSelectedObject() const;

        bool        mVisualizeAABBs      = false;
        bool        mFocusSelectedObject = false;
        glm::vec4   mBoxColor            = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);

        SPtr<RHI::VulkanRHI>            mRHI;
        BoundingBoxDebugPass_Params     mInput;
        SPtr<RHI::GraphicsPipeline2>    mPipeline;
    };

    class BoundingBoxDebugPassUI : public IComponent
    {
    public:
        explicit BoundingBoxDebugPassUI(BoundingBoxDebugPass* pPass);

        void draw() override;

    private:
        BoundingBoxDebugPass* mPass;
    };
}
