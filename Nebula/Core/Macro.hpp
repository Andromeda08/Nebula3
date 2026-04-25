#pragma once

#include "Types.hpp"

#define nbl_DECL_CTOR(T) \
    explicit T(const T##CreateInfo& createInfo);

#define nbl_CREATE_UNIQUE(T)                                    \
    static UPtr<T> create(const T##CreateInfo& createInfo) {    \
        return std::make_unique<T>(createInfo);                 \
    }

#define nbl_CREATE_SHARED(T)                                    \
    static SPtr<T> create(const T##CreateInfo& createInfo) {    \
        return std::make_shared<T>(createInfo);                 \
    }

#define nbl_CTOR(T)         \
    nbl_DECL_CTOR(T)        \
    nbl_CREATE_UNIQUE(T)

#define nbl_CTOR_SHARED(T)  \
    nbl_DECL_CTOR(T);       \
    nbl_CREATE_SHARED(T);

#define nbl_DISABLE_COPY(T)             \
    T(const T&) = delete;               \
    T& operator=(const T&) = delete;    \
    T(const T&&) = delete;              \
    T& operator=(const T&&) = delete;

#define nbl_DisableCopy(T) nbl_DISABLE_COPY(T)

#define nbl_CreateWithStruct(ClassType, ParamsType)             \
    static UPtr<ClassType> create(const ParamsType& params) {   \
        return std::make_unique<ClassType>(params);             \
    }
