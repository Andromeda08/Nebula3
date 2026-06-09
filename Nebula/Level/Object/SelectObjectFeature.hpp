#pragma once

#include <cstdint>

#include "Level/Raytracing/TLASSystem.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

// SelectObjectFeature.hpp
// Allows for picking objects by using Ray Queries to cast
// a ray into the bound Top Level AS. (requires RT)
// ============================================================

namespace nbl
{
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
        SelectObjectFeature(const SPtr<RHI::VulkanRHI>& rhi, CameraSystem* pCameraSystem, InstanceSystem* pInstanceSystem, TLASSystem* pTlasSystem)
        : mRHI(rhi)
        , mCameraSystem(pCameraSystem)
        , mInstanceSystem(pInstanceSystem)
        , mTlasSystem(pTlasSystem)
        {
            if (!mRHI->getFeatures().rayTracing)
            {
                spdlog::warn("Object selection feature is not available.");
                return;
            }

            mObjSelectBuffer = mRHI->createBuffer({
                .size  = sizeof(int32_t),
                .type  = RHI::BufferType::Readback,
                .label = "ObjSelectBuffer",
            });

            auto pipelineInfo = RHI::ComputePipelineCreateInfo()
                .addDescriptorSetLayout(mTlasSystem->getDescriptor()->getLayout())
                .setComputeShader(Configuration::getShaderFilePath("RQSelect.comp.spv"))
                .setDebugName("ObjSelectPipeline")
                .setPushConstantRange<PushConstant>(vk::ShaderStageFlagBits::eCompute);
            mObjSelectPipeline = mRHI->createComputePipeline(pipelineInfo);
        }

        void onEvent(const SDL_Event& event) noexcept
        {
            if (!mObjSelectPipeline)
            {
                return;
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if (const auto& mouseEvent = event.button; mouseEvent.button == SDL_BUTTON_RIGHT)
                {
                    mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void {
                        const auto [w, h] = mRHI->getSwapchain()->getProperties().extent;
                        glm::vec2 mousePos;
                        SDL_GetMouseState(&mousePos.x, &mousePos.y);

                        const auto pushConstants = PushConstant {
                            .instanceAddress = mInstanceSystem->getBuffer()->getAddress(),
                            .cameraAddress   = mCameraSystem->getBuffer(0)->getAddress(),
                            .selectAddress   = mObjSelectBuffer->getAddress(),
                            .mousePos        = std::move(mousePos),
                            .screenSize      = glm::vec2(w, h),
                        };

                        mObjSelectPipeline->bind(pCommandList);
                        mObjSelectPipeline->bindDescriptorSet(pCommandList, mTlasSystem->getDescriptor()->getSet(0));
                        mObjSelectPipeline->pushConstants(pCommandList, &pushConstants);
                        mObjSelectPipeline->dispatch(pCommandList, 1);

                        RHI::Barrier()
                            .addBarrier(mObjSelectBuffer->getBarrier(RHI::BufferUsage::Compute_Write, RHI::BufferUsage::Host_Read))
                            .insert(pCommandList);
                    });

                    const auto* pSelectedObj = static_cast<int32_t*>(mObjSelectBuffer->map());
                    mSelectedObject = pSelectedObj ? *pSelectedObj : -1;

                    spdlog::info("Selected object: {}", mSelectedObject);
                }
            }
        }

        [[nodiscard]] int32_t* getSelectedObjectIdx() noexcept
        {
            return &mSelectedObject;
        }

    private:
        SPtr<RHI::VulkanRHI>        mRHI;
        CameraSystem*               mCameraSystem;
        InstanceSystem*             mInstanceSystem;
        TLASSystem*                 mTlasSystem;

        int32_t                     mSelectedObject = -1;
        SPtr<RHI::Buffer>           mObjSelectBuffer;
        SPtr<RHI::ComputePipeline>  mObjSelectPipeline;
    };
}