#include "Gamepad.hpp"

#include <spdlog/spdlog.h>

#include "DualSense.hpp"
#include "Core/Util.hpp"

Gamepad::Gamepad(const SDL_JoystickID gamepadId)
: mId(gamepadId)
{
    mName    = SDL_GetGamepadNameForID(mId);
    mVendor  = SDL_GetGamepadVendorForID(mId);
    mProduct = SDL_GetGamepadProductForID(mId);

    mGamepad = SDL_OpenGamepad(mId);

    const SDL_PropertiesID props = SDL_GetGamepadProperties(mGamepad);
    mHasLED = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_RGB_LED_BOOLEAN, false);

    setLEDColor(255, 255, 0);

    spdlog::info("Gamepad connected: {} (Vendor: 0x{:04X}  Product: 0x{:04X})", mName, mVendor, mProduct);
}

Gamepad::~Gamepad()
{
    if (mHasLED)
    {
        setLEDColor(0, 0, 255);
    }

    SDL_CloseGamepad(mGamepad);
    spdlog::info("Gamepad disconnected: {}", mName);
}

void Gamepad::setLEDColor(const uint8_t r, const uint8_t g, const uint8_t b) const noexcept
{
    if (mHasLED)
    {
        SDL_SetGamepadLED(mGamepad, r, g, b);
    }
}

SDL_JoystickID Gamepad::getId() const noexcept
{
    return mId;
}

SDL_Gamepad* Gamepad::getHandle() const noexcept
{
    return mGamepad;
}

GamepadManager::GamepadManager()
{
    Input::Gamepad::Mappings::DualSense::registerMapping();

    // Ensure Gamepad subsystem is initialized
    if (!SDL_WasInit(SDL_INIT_GAMEPAD))
    {
        spdlog::warn("SDL Gamepad system wasn't initialized yet.");
        if (!SDL_Init(SDL_INIT_GAMEPAD))
        {
            exitWithError("Failed to initialize SDL Gamepad subsystem");
        }
    }

    // Select initial Gamepad (if available)
    int32_t nJoysticks = 0;
    if (SDL_JoystickID* joysticks = SDL_GetJoysticks(&nJoysticks))
    {
        for (int32_t i = 0; i < nJoysticks; ++i)
        {
            if (SDL_IsGamepad(joysticks[i]))
            {
                makeActive(joysticks[i]);
            }
        }
        SDL_free(joysticks);
    }

    if (!mGamepad)
    {
        spdlog::info("No gamepads are connected");
    }
}

void GamepadManager::onGamepadEvent(const SDL_Event& event) noexcept
{
    switch (event.type)
    {
        case SDL_EVENT_GAMEPAD_ADDED: {
            makeActive(event.gdevice.which);
            break;
        }
        case SDL_EVENT_GAMEPAD_REMOVED: {
            removeGamepad(event.gdevice.which);
            break;
        }
        default: {
            break;
        }
    }
}

Gamepad* GamepadManager::getActiveGamepad() const noexcept
{
    return mGamepad.get();
}

void GamepadManager::makeActive(const SDL_JoystickID id)
{
    mGamepad = makeUnique<Gamepad>(id);
}

void GamepadManager::removeGamepad(const SDL_JoystickID& joystickId) noexcept
{
    if (!mGamepad)
    {
        return;
    }
    if (joystickId != mGamepad->getId())
    {
        return;
    }

    mGamepad.reset();
}
