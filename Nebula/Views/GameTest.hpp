#pragma once

#include <imgui.h>

#include "Core/Random.hpp"
#include "Core/View.hpp"
#include "Interface/Interface.hpp"

namespace nbl
{
    enum class GameState
    {
        Menu,
        Playing,
    };

    class GameTest : public View
    {
        struct r_CursorTestPushConstants
        {
            glm::mat4 proj;
            glm::vec2 center;
            float     a;
            glm::vec2 viewportSize;
            int32_t   cursorTextureIndex;
            glm::vec3 color;
            int32_t   isHit;
            float     fade;
        };
    public:
        GameTest(nbl_ViewCtorParams)
        : nbl_ViewBaseCtor
        , mState(GameState::Menu)
        {
            mName = "rhythm";
            InterfaceParams ip = {
                { 1920.0f, 1080.0f },
                { 64.0f, 64.0f },
                mRHI,
                mTextureManager,
                {},
                true
            };
            mInterface = makeUnique<Interface>(ip);

            mOrthoProj = glm::orthoLH_ZO(0.0f, 1920.0f, 0.0f, 1080.0f, 0.0f, 1.0f);

            using enum vk::ShaderStageFlagBits;
            const auto graphicsPS = RHI::GraphicsPS()
                .setCullMode(vk::CullModeFlagBits::eNone)
                .addAlphaAttachmentState(1)
                .addAttachmentFormat(mRHI->getSwapchain()->getProperties().format)
                .setTopology(vk::PrimitiveTopology::eTriangleStrip);
            const auto pipelineInfo = RHI::PipelineCommon()
                .setLabel("r_CursorTest")
                .addShader("r_CursorTest.vert.spv")
                .addShader("r_CursorTest.frag.spv")
                .addDescriptorLayout(0, mTextureManager->getDescriptor().get())
                .setPushConstant<r_CursorTestPushConstants>(eVertex | eFragment);
            mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);

            mCursorTextureIndex = static_cast<int32_t>(mTextureManager->loadTexture("cursor.png"));
        }

        ~GameTest() override = default;

        void onEvent(const SDL_Event& event) override
        {
            SDL_GetMouseState(&mMousePos.x, &mMousePos.y);

            switch (event.type)
            {
                case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        mMbLeftDown = true;
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        mMbLeftDown = false;
                    }
                    break;
                }
                default: break;
            }

            if (mState == GameState::Menu)
            {
                switch (event.type)
                {
                    case SDL_EVENT_KEY_DOWN:
                    case SDL_EVENT_KEY_UP: {
                        case SDL_SCANCODE_SPACE: {
                            mState = GameState::Playing;
                            SDL_HideCursor();
                            break;
                        }
                    }
                    default: break;
                }
            }
        }

        void onUpdate(const float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override
        {
            if (mState != GameState::Playing)
            {
                return;
            }

            static float     accumTime        = 0.0f;
            constexpr float  cSpawnInterval   = 0.45f;
            static glm::vec2 lastSpawnPoint   = glm::vec2(0.0f);
            static float     dtSinceLastSpawn = 0.0f;

            dtSinceLastSpawn += dt;

            // Check hits
            for (uint32_t i = 0; i < mPositions.size(); i++)
            {
                const float delta = accumTime - mSpawnTime[i];

                const auto bbox = BoundingBox(
                    glm::vec3(mPositions[i] - glm::vec2(mSize / 2.0f), 0.0f),
                    glm::vec3(mPositions[i] + glm::vec2(mSize / 2.0f), 0.0f));

                if (!mWasHit[i])
                {
                    mWasHit[i] = mMbLeftDown && bbox.contains(glm::vec3(mMousePos, 0.0f));
                    if (mWasHit[i])
                    {
                        if (delta <= 0.5f)
                        {
                            mScore += 300;
                        }
                        else if (delta <= 1.0f)
                        {
                            mScore += 100;
                        }
                        else
                        {
                            mScore += 50;
                        }
                    }
                }

                if (mWasHit[i])
                {
                    mFadeState[i] = std::max(0.0f, mFadeState[i] - 1.0f * dt);
                }
            }

            // Do spawn
            if (dtSinceLastSpawn >= cSpawnInterval)
            {
                glm::vec2 candidate;
                while (glm::length(candidate - lastSpawnPoint) <= 64.0f)
                {
                    constexpr float xPad = 1920.0f - 1080.0f / 2.0f - 32.0f;
                    candidate = {
                        Random::get(xPad + mSize, 1920.0f - xPad - mSize),
                        Random::get(32.0f + mSize, 1080.0f - 32.0f - mSize),
                    };
                }

                mPositions.push_back(candidate);
                mColors.push_back(glm::xyz(Random::getColor()));
                mWasHit.push_back(false);
                mFadeState.push_back(1.0f);
                mSpawnTime.push_back(accumTime);

                dtSinceLastSpawn = 0.0f;
            }

            accumTime += dt;
        }

        void onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) override
        {
            if (mState == GameState::Menu)
            {
                mInterface->render(pCommandList, frameData);
                pCommandList->blitToSwapchain(mInterface->getResult(frameData.currentFrame).get(), mRHI->getSwapchain(), frameData.acquiredIndex);
            }
            if (mState == GameState::Playing)
            {
                RHI::Rendering()
                    .setLabel("r_CursorTest")
                    .setRenderArea(mRHI->getSwapchain()->getProperties().extent)
                    .addAttachment(mRHI->getSwapchain()->getImageRHI(frameData.acquiredIndex))
                    .setViewportScissor(pCommandList)
                    .insertBarriers(pCommandList)
                    .execute(pCommandList, [&](RHI::CommandList* cmd)
                    {
                        auto pcs = r_CursorTestPushConstants {
                            .proj               = mOrthoProj,
                            .center             = mMousePos,
                            .a                  = 100.0f,
                            .viewportSize       = { 1920.0f, 1080.0f },
                            .cursorTextureIndex = mCursorTextureIndex,
                            .color              = glm::vec3(1.0f),
                            .isHit              = 0,
                            .fade               = 1.0f,
                        };
                        cmd->bindPipeline(mPipeline.get());
                        cmd->pushConstants(&pcs);
                        cmd->bindDescriptorSet(mTextureManager->getDescriptor()->getSet());
                        cmd->draw(4, 1, 0, 0);

                        for (uint32_t i = 0; i < mPositions.size(); i++)
                        {
                            pcs.cursorTextureIndex = -1;
                            pcs.center = mPositions[i];
                            pcs.a      = mSize;
                            pcs.color  = mColors[i];
                            pcs.isHit  = mWasHit[i] ? 1 : 0;
                            pcs.fade   = std::max(0.0f, mFadeState[i]);

                            cmd->pushConstants(&pcs);
                            cmd->draw(4, 1, 0, 0);
                        }
                    });
            }
        }

        void onDrawUI() override
        {
            ImGui::Begin("Stats");
            ImGui::Text("Score: %llu", mScore);
            ImGui::End();
        }

    private:
        bool                            mMbLeftDown = false;
        uint64_t                        mScore      = 0;
        float                           mSize       = 64.0f;
        std::vector<glm::vec2>          mPositions;
        std::vector<glm::vec3>          mColors;
        std::vector<bool>               mWasHit;
        std::vector<float>              mFadeState;
        std::vector<float>              mSpawnTime;

        glm::vec2                       mMousePos;

        int32_t                         mCursorTextureIndex;
        glm::mat4                       mOrthoProj;
        UPtr<RHI::GraphicsPipeline2>    mPipeline;

        GameState                       mState;
        UPtr<Interface>                 mInterface;
    };
}
