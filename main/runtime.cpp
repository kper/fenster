//
// Created by Johannes Zottele on 04.12.25.
//
// Freestanding C++ runtime support
// Provides necessary operators that would normally come from libstdc++

#include <stddef.h>

// Placement new operator
void* operator new(size_t, void* ptr) noexcept {
    return ptr;
}

// Delete operators (no-ops in freestanding environment without heap)
void operator delete(void*) noexcept {
    // No-op: we don't have a heap allocator
}

void operator delete(void*, unsigned long) noexcept {
    // No-op: sized deallocation variant
}
