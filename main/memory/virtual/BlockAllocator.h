//
// Created by Johannes Zottele on 08.12.25.
//

#ifndef MAIN_BLOCKALLOCATOR_H
#define MAIN_BLOCKALLOCATOR_H
#include "LinkedListAllocator.h"
#include "VirtualAllocator.h"

namespace memory {

    static constexpr size_t BLOCK_SIZES[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2028};
    static constexpr uint8_t BLOCK_SIZE_COUNT = sizeof(BLOCK_SIZES) / sizeof(size_t);

    class BlockAllocator;

    static class BlockNode {
        friend BlockAllocator;
        BlockNode* next;
    public:
        explicit BlockNode(BlockNode* next): next(next) {};
    };

    class BlockAllocator: public Allocator{
        BlockNode* heads[BLOCK_SIZE_COUNT];
        LinkedListAllocator fallback_allocator;
    public:
        BlockAllocator(): heads{nullptr}, fallback_allocator(LinkedListAllocator()) {}
        void init(size_t heap_start, size_t heap_size);
        void *allocate(size_t size, size_t align) override;
        void deallocate(void *ptr, size_t size) override;

    private:
        void *fallback_alloc(size_t size, size_t alignment);
    };

}

#endif //MAIN_BLOCKALLOCATOR_H