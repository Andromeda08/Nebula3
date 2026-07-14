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
        SelectObjectFeature(const SPtr<RHI::VulkanRHI>& rhi, CameraSystem* pCameraSystem, InstanceSystem* pInstanceSystem, TLASSystem* pTlasSystem, const PrePass* pPrePass)
        : mRHI(rhi)
        , mCameraSystem(pCameraSystem)
        , mInstanceSystem(pInstanceSystem)
        , mTlasSystem(pTlasSystem)
        {
            mObjSelectBuffer = mRHI->createBuffer({
                .size  = sizeof(int32_t),
                .type  = RHI::BufferType::Readback,
                .label = "ObjSelectBuffer",
            });

            if (mRHI->getFeatures().rayTracing)
            {
                auto pipelineInfo = RHI::ComputePipelineCreateInfo()
                    .addDescriptorSetLayout(mTlasSystem->getDescriptor()->getLayout())
                    .setComputeShader(Configuration::getShaderFilePath("RQSelect.comp.spv"))
                    .setDebugName("ObjSelectPipeline")
                    .setPushConstantRange<PushConstant>(vk::ShaderStageFlagBits::eCompute);
                mObjSelectPipeline = mRHI->createComputePipeline(pipelineInfo);
            }
            else
            {
                if (pPrePass == nullptr)
                {
                    spdlog::warn("Object selection feature unavailable: No RT support or PrePass to use.");
                    return;
                }

                mDescriptor = mRHI->createDescriptor({
                    .bindings = {{ 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute }},
                    .setCount = RHI::gFramesInFlight,
                    .debugName = "ObjSelect_Descriptor",
                });

                for (uint32_t i = 0; i < RHI::gFramesInFlight; i++)
                {
                    auto write = RHI::DescriptorWrite()
                        .writeStorageImage(0, vk::ImageLayout::eGeneral, pPrePass->getObjInstanceBuffer(i));
                    mDescriptor->write(i, write);
                }

                auto pipelineInfo = RHI::ComputePipelineCreateInfo()
                    .addDescriptorSetLayout(mDescriptor->getLayout())
                    .setComputeShader(Configuration::getShaderFilePath("Select.comp.spv"))
                    .setDebugName("ObjSelectPipeline")
                    .setPushConstantRange<PushConstant>(vk::ShaderStageFlagBits::eCompute);
                mObjSelectPipeline = mRHI->createComputePipeline(pipelineInfo);
            }
        }

        void onEvent(const SDL_Event& event) noexcept;

        [[nodiscard]] int32_t* getSelectedObjectIdx() noexcept;

    private:
        SPtr<RHI::VulkanRHI>        mRHI;
        CameraSystem*               mCameraSystem;
        InstanceSystem*             mInstanceSystem;
        TLASSystem*                 mTlasSystem;

        SPtr<RHI::Descriptor>       mDescriptor;

        int32_t                     mSelectedObject = -1;
        SPtr<RHI::Buffer>           mObjSelectBuffer;
        SPtr<RHI::ComputePipeline>  mObjSelectPipeline;
    };
}