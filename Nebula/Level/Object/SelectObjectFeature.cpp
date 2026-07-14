#include "SelectObjectFeature.hpp"

namespace nbl
{
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
                    mousePos *= SDL_GetWindowPixelDensity(SDL_GetWindowFromID(mouseEvent.windowID));
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

    int32_t* SelectObjectFeature::getSelectedObjectIdx() noexcept
    {
        return &mSelectedObject;
    }
}
