//
// Created by Johannes Zottele on 05.12.25.
//

#ifndef MAIN_NEW_H
#define MAIN_NEW_H

#include <stddef.h>

inline void* operator new(size_t, void* p) noexcept { return p; }
inline void* operator new[](size_t, void* p) noexcept { return p; }

inline void operator delete(void*, void*) noexcept {}
inline void operator delete[](void*, void*) noexcept {}

#endif //MAIN_NEW_H