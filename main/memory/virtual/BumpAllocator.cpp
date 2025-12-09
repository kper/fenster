//
// Created by Johannes Zottele on 06.12.25.
//

#include "BumpAllocator.h"

namespace memory {

    BumpAllocator::BumpAllocator(VirtualAddress heap_start, VirtualAddress heap_end)
        : heap_start(heap_start), heap_end(heap_end), next(AtomicU64(heap_start)) {}

    void *BumpAllocator::allocate(size_t size, size_t align) {
        while (true) {
            auto curr_next = next.load();
            auto alloc_start = align_up(curr_next, align);
            auto alloc_end = saturating_add(alloc_start, size);

            if (alloc_end <= heap_end) {
                auto next_now = next.compare_and_swap(curr_next, alloc_end);
                if (next_now == curr_next) {
                    // swap was successful as the old value was returned
                    return reinterpret_cast<void*>(alloc_start);
                }
                // some other thread already swapped it, we must try again.
            } else {
                // the heap is exhausted
                return nullptr;
            }
        }
    }

    void  BumpAllocator::deallocate(void *ptr, size_t size) {
        // do nothing, we leak all allocated memory
    }
}