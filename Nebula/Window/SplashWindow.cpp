#include "SplashWindow.hpp"

#include "Core/Ranges.hpp"

void SplashWindow::setMessage(const std::string& message, const std::string& sceneName) const
{
    if (!mWindow || !mFont)
    {
        return;
    }

    SDL_PumpEvents();

    const auto* keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_ESCAPE])
    {
        std::exit(0);
    }

    int w, h;
    SDL_GetWindowSize(mWindow, &w, &h);

    SDL_SetRenderDrawColor(mRenderer, 25, 25, 25, 255);
    SDL_RenderClear(mRenderer);

    constexpr SDL_Color white = { 255, 255, 255, 255 };

    std::array<SDL_Surface*, 3> surfaces;
    std::array<SDL_Texture*, 3> textures;
    std::array<SDL_FRect,    3> dstRect;

    /* Live Message */
    SDL_Surface* surface0 = TTF_RenderText_Blended(mFont, message.c_str(), 0, white);
    SDL_Texture* texture0 = SDL_CreateTextureFromSurface(mRenderer, surface0);

    SDL_FRect dst0 = {
        10.0f,
        static_cast<float>(h) - static_cast<float>(surface0->h) - 10.0f,
        static_cast<float>(surface0->w),
        static_cast<float>(surface0->h)
    };

    /* Scene Name */
    SDL_Surface* surface1 = nullptr;
    SDL_Texture* texture1 = nullptr;
    SDL_FRect    dst1     = {};
    if (!sceneName.empty())
    {
        surface1 = TTF_RenderText_Blended(mFont, sceneName.c_str(), 0, white);
        texture1 = SDL_CreateTextureFromSurface(mRenderer, surface1);

        dst1 = {
            10.0f,
            static_cast<float>(h) - static_cast<float>(surface0->h) - 10.0f - static_cast<float>(surface1->h) - 10.0f,
            static_cast<float>(surface1->w),
            static_cast<float>(surface1->h)
        };
    }

    /* Engine Name */
    SDL_Surface* surface2 = TTF_RenderText_Blended(mFont16, "Nebula", 0, white);
    SDL_Texture* texture2 = SDL_CreateTextureFromSurface(mRenderer, surface2);

    SDL_FRect dst2 = {
        10.0f,
        static_cast<float>(surface2->h) - 10.0f,
        static_cast<float>(surface2->w),
        static_cast<float>(surface2->h)
    };

    SDL_RenderTexture(mRenderer, texture0, nullptr, &dst0);
    if (!sceneName.empty())
    {
        SDL_RenderTexture(mRenderer, texture1, nullptr , &dst1);
    }
    SDL_RenderTexture(mRenderer, texture2, nullptr, &dst2);
    SDL_RenderPresent(mRenderer);

    SDL_DestroyTexture(texture0);
    SDL_DestroySurface(surface0);
    if (!sceneName.empty())
    {
        SDL_DestroyTexture(texture1);
        SDL_DestroySurface(surface1);
    }
    SDL_DestroyTexture(texture2);
    SDL_DestroySurface(surface2);
}
