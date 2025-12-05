//
// Created by Johannes Zottele on 04.12.25.
//
// Freestanding C++ runtime support
// Provides necessary operators that would normally come from libstdc++

#include <stddef.h>

inline void* operator new(size_t, void* p) noexcept { return p; }
inline void* operator new[](size_t, void* p) noexcept { return p; }

inline void operator delete(void*, void*) noexcept {}
inline void operator delete[](void*, void*) noexcept {}