#include "SelectObjectFeature.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

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
            auto pipelineInfo = RHI::PipelineCommon()
                .addDescriptorLayout(0, mTlasSystem->getDescriptor().get())
                .addShader("RQSelect.comp.spv")
                .setLabel("ObjSelectPipeline")
                .setPushConstant<PushConstant>(vk::ShaderStageFlagBits::eCompute);
            mObjSelectPipeline = mRHI->createComputePipeline2(pipelineInfo);
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

            auto pipelineInfo = RHI::PipelineCommon()
                .addDescriptorLayout(0, mDescriptor.get())
                .addShader("Select.comp.spv")
                .setLabel("ObjSelectPipeline")
                .setPushConstant<PushConstant>(vk::ShaderStageFlagBits::eCompute);
            mObjSelectPipeline = mRHI->createComputePipeline2(pipelineInfo);
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
                mRHI->getGraphicsQueue()->immediate([&](RHI::CommandList* pCommandList) -> void {
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

                    pCommandList->bindPipeline(mObjSelectPipeline.get());

                    if (mRHI->getFeatures().rayTracing)
                    {
                        pCommandList->bindDescriptorSet(mTlasSystem->getDescriptor()->getSet(0), 0);
                    }
                    else
                    {
                        pCommandList->bindDescriptorSet(mDescriptor->getSet(0), 0);
                    }

                    pCommandList->pushConstants(&pushConstants);
                    pCommandList->dispatch();

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

    void SelectObjectFeature::onDrawUI(const std::vector<UPtr<Object>>& objects)
    {
        static ImGuizmo::OPERATION gizmoOp   = ImGuizmo::TRANSLATE;
        static ImGuizmo::MODE      gizmoMode = ImGuizmo::WORLD;

        ImGui::Begin("Object Editor");

        if (ImGui::RadioButton("Translate", gizmoOp == ImGuizmo::TRANSLATE))
        {
            gizmoOp = ImGuizmo::TRANSLATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", gizmoOp == ImGuizmo::ROTATE))
        {
            gizmoOp = ImGuizmo::ROTATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", gizmoOp == ImGuizmo::SCALE))
        {
            gizmoOp = ImGuizmo::SCALE;
        }

        if (mSelectedObject == -1)
        {
            ImGui::Text("No selected object.");
        }
        else
        {
            if (ImGui::SmallButton("Clear Selection"))
            {
                mSelectedObject = -1;
                ImGui::End();
                return;
            }

            auto* pSelectedObject = objects[mSelectedObject].get();
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("Name", &pSelectedObject->name);

            auto& tf = pSelectedObject->transform;

            float translate[3] = { tf._translate.x, tf._translate.y, tf._translate.z };
            float rotate[3]    = { tf._euler.x,     tf._euler.y,     tf._euler.z     };
            float scale[3]     = { tf._scale.x,     tf._scale.y,     tf._scale.z     };

            bool edited = false;
            edited |= ImGui::InputFloat3("T", translate);
            edited |= ImGui::InputFloat3("R", rotate);
            edited |= ImGui::InputFloat3("S", scale);

            if (edited)
            {
                tf.setTranslate({ translate[0], translate[1], translate[2] })
                  .setRotation ({ rotate[0],    rotate[1],    rotate[2]    })
                  .setScale    ({ scale[0],     scale[1],     scale[2]     });
                pSelectedObject->isInstanceDirty = true;
            }

            auto matrix = tf.getModel();

            auto cameraData = mCameraSystem->getActiveCamera()->getGpuCameraData();
            cameraData.proj[1][1] *= -1.0f;

            ImGuiIO& io = ImGui::GetIO();
            ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
            const bool gizmoEdited = ImGuizmo::Manipulate(glm::value_ptr(cameraData.view), glm::value_ptr(cameraData.proj),
                gizmoOp, gizmoMode, glm::value_ptr(matrix), nullptr, nullptr);

            if (gizmoEdited)
            {
                glm::vec3 t, s, skew;
                glm::quat q;
                glm::vec4 persp;
                glm::decompose(matrix, s, q, t, skew, persp);

                float yaw, pitch, roll;
                glm::extractEulerAngleYXZ(glm::mat4_cast(q), yaw, pitch, roll);

                pSelectedObject->transform
                    .setTranslate(t)
                    .setRotation(glm::degrees(glm::vec3(pitch, yaw, roll)))
                    .setScale(s);
                pSelectedObject->isInstanceDirty = true;
            }
        }

        /*
        else
        {

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
        */

        ImGui::End();
    }

    int32_t* SelectObjectFeature::getSelectedObjectIdx() noexcept
    {
        return &mSelectedObject;
    }
}
