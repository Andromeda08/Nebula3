#include "SelectObjectFeature.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "Object.hpp"

namespace nbl
{
    SelectObjectFeature::SelectObjectFeature(
        const SPtr<RHI::VulkanRHI>& rhi,
        CameraSystem*               pCameraSystem,
        InstanceSystem*             pInstanceSystem,
        TLASSystem*                 pTlasSystem,
        const PrePass*              pPrePass)
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

    void SelectObjectFeature::onEvent(const SDL_Event& event) noexcept
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
                    const auto [w, h]   = mRHI->getSwapchain()->getProperties().extent;
                    glm::vec2  mousePos = { mouseEvent.x, mouseEvent.y };
                    spdlog::info("Select at: [{}, {}]", mousePos.x, mousePos.y);
                    // mousePos *= SDL_GetWindowPixelDensity(SDL_GetWindowFromID(mouseEvent.windowID));
                    // SDL_GetMouseState(&mousePos.x, &mousePos.y);
                    // mousePos *= gWindow->getDisplayScale();

                    const auto pushConstants = PushConstant {
                        .instanceAddress = mInstanceSystem->getBuffer()->getAddress(),
                        .cameraAddress   = mCameraSystem->getBuffer(0)->getAddress(),
                        .selectAddress   = mObjSelectBuffer->getAddress(),
                        .mousePos        = mousePos,
                        .screenSize      = glm::vec2(w, h),
                    };

                    mObjSelectPipeline->bind(pCommandList);

                    if (mRHI->getFeatures().rayTracing)
                    {
                        mObjSelectPipeline->bindDescriptorSet(pCommandList, mTlasSystem->getDescriptor()->getSet(0));
                    }
                    else
                    {
                        mObjSelectPipeline->bindDescriptorSet(pCommandList, mDescriptor->getSet(0));
                    }

                    mObjSelectPipeline->pushConstants(pCommandList, &pushConstants);
                    mObjSelectPipeline->dispatch(pCommandList, 1);

                    RHI::Barrier()
                        .addBarrier(mObjSelectBuffer->getBarrier(RHI::BufferUsage::Compute_Write, RHI::BufferUsage::Host_Read))
                        .insert(pCommandList);
                });

                const auto* pSelectedObj = static_cast<int32_t*>(mObjSelectBuffer->map());
                mSelectedObject          = pSelectedObj ? *pSelectedObj : -1;

                spdlog::info("Selected object: {}", mSelectedObject);
            }
        }
    }

    void SelectObjectFeature::onDrawUI(const std::vector<UPtr<Object>>& objects) const
    {
        ImGui::Begin("Object Editor");

        if (mSelectedObject == -1)
        {
            ImGui::Text("none");
        }
        else
        {
            auto* pSelectedObject = objects[mSelectedObject].get();
            ImGui::Text("%s", pSelectedObject->name.c_str());
            if (ImGui::CollapsingHeader("Transform"))
            {
                bool dirty = false;
                dirty |= ImGui::DragFloat3("Position", glm::value_ptr(pSelectedObject->transform._translate), 1.0f, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
                dirty |= ImGui::DragFloat3("Scale", glm::value_ptr(pSelectedObject->transform._scale), 0.1f, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
                dirty |= ImGui::DragFloat3("Rotation", glm::value_ptr(pSelectedObject->transform._euler), 0.5f, -360.0f, 360.0f, "%.3f", ImGuiSliderFlags_WrapAround);
                if (dirty)
                {
                    pSelectedObject->isInstanceDirty = true;
                }
            }
        }

        ImGui::End();
    }

    int32_t* SelectObjectFeature::getSelectedObjectIdx() noexcept
    {
        return &mSelectedObject;
    }
}
