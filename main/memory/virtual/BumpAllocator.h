//
// Created by Johannes Zottele on 06.12.25.
//

#ifndef MAIN_BUMPALLOCATOR_H
#define MAIN_BUMPALLOCATOR_H

#include "VirtualAllocator.h"
#include "paging/paging.h"
#include "runtime/atomic.h"

namespace memory {
    class BumpAllocator {
        VirtualAddress heap_start;
        VirtualAddress heap_end;
        AtomicU64 next;

    public:
        BumpAllocator(VirtualAddress heap_start, VirtualAddress heap_end);
        void *allocate(size_t size, size_t align);
        void  deallocate(void *ptr, size_t size);
    };


}
#endif //MAIN_BUMPALLOCATOR_H