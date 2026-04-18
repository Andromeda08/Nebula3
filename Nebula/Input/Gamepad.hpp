#pragma once

#include <SDL3/SDL.h>

#include "Core/Types.hpp"

class Gamepad
{
public:
    explicit Gamepad(SDL_JoystickID gamepadId);

    ~Gamepad();

    void setLEDColor(uint8_t r, uint8_t g, uint8_t b) const noexcept;

    [[nodiscard]] SDL_JoystickID getId() const noexcept;

    [[nodiscard]] SDL_Gamepad* getHandle() const noexcept;

private:
    SDL_JoystickID  mId;
    SDL_Gamepad*    mGamepad = nullptr;

    bool            mHasLED;
    std::string     mName;
    uint16_t        mVendor;
    uint16_t        mProduct;
};

/**
 * Selects the first available Gamepad by default.
 * Gamepads added during runtime automatically become the active one.
 */
class GamepadManager
{
public:
    GamepadManager();

    // Listen for Gamepad connection events
    void onGamepadEvent(const SDL_Event& event) noexcept;

    [[nodiscard]] Gamepad* getActiveGamepad() const noexcept;

private:
    void makeActive(SDL_JoystickID id);

    void removeGamepad(const SDL_JoystickID& joystickId) noexcept;

    UPtr<Gamepad> mGamepad;
};
