#pragma once

#define nbl_DISABLE_COPY(T)             \
    T(const T&) = delete;               \
    T& operator=(const T&) = delete;    \
    T(const T&&) = delete;              \
    T& operator=(const T&&) = delete;
