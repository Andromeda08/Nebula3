#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

class SplashWindow
{
public:
    static SplashWindow& get()
    {
        static SplashWindow instance;
        return instance;
    }

    void open() noexcept
    {
        TTF_Init();
        mWindow  = SDL_CreateWindow("Nebula", 420, 120, SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP);
        mRenderer = SDL_CreateRenderer(mWindow, nullptr);
        mFont = TTF_OpenFont("Resources/Fonts/GeistMono-Regular.ttf", 12);
        mFont16 = TTF_OpenFont("Resources/Fonts/GeistMono-Regular.ttf", 16);
    }

    void close()
    {
        if (mFont)
        {
            TTF_CloseFont(mFont);
        }
        mFont = nullptr;
        if (mFont16)
        {
            TTF_CloseFont(mFont16);
        }
        mFont16 = nullptr;
        if (mRenderer)
        {
            SDL_DestroyRenderer(mRenderer);
        }
        mRenderer = nullptr;
        if (mWindow)
        {
            SDL_DestroyWindow(mWindow);
        }
        mWindow = nullptr;
    }

    void setMessage(const std::string& message, const std::string& sceneName = "") const;

private:
    SplashWindow()
    {
        SDL_Init(SDL_INIT_VIDEO);
    }

    SDL_Window*     mWindow   = nullptr;
    SDL_Renderer*   mRenderer = nullptr;
    TTF_Font*       mFont     = nullptr;
    TTF_Font*       mFont16   = nullptr;
};