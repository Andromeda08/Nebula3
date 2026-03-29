#pragma once

#include <SDL3/SDL_gamepad.h>

class DualSense
{
public:
    static void addGamepadMapping() noexcept
    {
        SDL_AddGamepadMapping(
            "050057564c050000e60c000000006800,"
            "DualSense Wireless Controller,"
            "type:6,"
            "a:b0,b:b1,back:b4,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,"
            "guide:b5,leftshoulder:b9,leftstick:b7,lefttrigger:a4,leftx:a0,lefty:a1,"
            "rightshoulder:b10,rightstick:b8,righttrigger:a5,rightx:a2,righty:a3,"
            "start:b6,x:b2,y:b3,touchpad:b11,misc1:b12,crc:5657,"
        );
    }

private:

};
