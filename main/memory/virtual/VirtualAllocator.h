//
// Created by Johannes Zottele on 06.12.25.
//

#ifndef MAIN_VIRTUALALLOCATOR_H
#define MAIN_VIRTUALALLOCATOR_H

#include <stdint.h>
#include <stddef.h>

#include "paging/paging.h"

namespace memory {
    constexpr size_t HEAP_START = 0000'001'000'000'0000 + paging::KERNEL_OFFSET;
    constexpr size_t HEAP_SIZE = 100 * 1024; // 100 KiB

    // Note: Removed Allocator base class to avoid vtables
    // BlockAllocator and LinkedListAllocator are now concrete types

    inline size_t is_power_of_2(size_t size) {
        return size != 0 && (size & (size - 1)) == 0;
    }

    inline uint64_t saturating_add(uint64_t a, uint64_t b) {
        uint64_t r = a + b;
        if (r < a) return UINT64_MAX;
        return r;
    }

    inline size_t align_down(VirtualAddress addr, size_t align) {
        if (is_power_of_2(align)) {
            return addr & ~(align - 1);
        }
        if (align == 0) {
            return addr;
        }
        PANIC("Alignment must be power of 2");
    }

    inline size_t align_up(VirtualAddress addr, size_t align) {
        return (addr + align - 1) & ~(align - 1);
    }

}

#endif //MAIN_VIRTUALALLOCATOR_H