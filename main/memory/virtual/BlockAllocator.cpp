//
// Created by Johannes Zottele on 08.12.25.
//

#include "BlockAllocator.h"

namespace memory {

    rnt::Optional<size_t> size_index(size_t size, size_t alignment);

    void BlockAllocator::init(size_t heap_start, size_t heap_size) {
        fallback_allocator.init(heap_start, heap_size);
    }

    void *BlockAllocator::fallback_alloc(size_t size, size_t alignment) {
        return fallback_allocator.allocate(size, alignment);
    }

    void *BlockAllocator::allocate(size_t size, size_t align) {
        auto index = size_index(size, align);
        if (index.is_empty()) {
            return fallback_alloc(size, align);
        }

        auto i = index.value();
        auto head = heads[i];
        if (head) {
            // we have a free block for the required size.
            // remove the block from the free ones
            heads[i] = head->next;
            // return the selected block
            return head;
        }

        // we don't have a free block for the minimal desired block size.
        // we must allocate a new block with the desired size
        auto block_size = BLOCK_SIZES[i];
        // for simplicity, a block's alignment is its size
        auto block_align = block_size;
        // allocate the block with the fallback allocator.
        // when the block is freed again, we will add it to the heads
        return fallback_allocator.allocate(block_size, block_align);
    }

    void BlockAllocator::deallocate(void *ptr, size_t size) {
        auto index = size_index(size, size);
        if (index.is_empty()) {
            // was allocated via the fallback allocator
            fallback_allocator.deallocate(ptr, size);
            return;
        }

        auto i = index.value();
        // create a new block node that points to the current head node
        // at the returned deallocation ptr.
        auto new_node = reinterpret_cast<BlockNode*>(ptr);
        *new_node = BlockNode(heads[i]);
        heads[i] = new_node;
    }



    rnt::Optional<size_t> size_index(size_t size, size_t alignment) {
        auto required_block_size = size > alignment ? size : alignment;
        for (int i = 0; i < BLOCK_SIZE_COUNT; i++) {
            if (BLOCK_SIZES[i] >= required_block_size) {
                return i;
            }
        }
        return {};
    }

}
