#include "DeltaTime.hpp"

#include <cassert>

DeltaTime& DeltaTime::initialize() noexcept
{
    mReady    = true;
    mLastTime = std::chrono::high_resolution_clock::now();
    return *this;
}

float DeltaTime::getDeltaTime() noexcept
{
    assert(mReady);

    const auto currentTime = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> delta = currentTime - mLastTime;
    const float dt = delta.count();

    mLastTime = currentTime;

    return dt;
}
