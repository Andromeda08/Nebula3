#pragma once

#include <chrono>

class DeltaTime
{
public:
    DeltaTime& initialize() noexcept;

    // Get delta time in seconds.
    float getDeltaTime() noexcept;

private:
    bool                                           mReady = false;
    std::chrono::high_resolution_clock::time_point mLastTime;
};
