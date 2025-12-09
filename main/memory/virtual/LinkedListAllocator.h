//
// Created by Johannes Zottele on 08.12.25.
//

#ifndef MAIN_LINKEDLISTALLOCATOR_H
#define MAIN_LINKEDLISTALLOCATOR_H
#include "VirtualAllocator.h"
#include "paging/paging.h"

namespace memory {

    class LinkedListAllocator;

    static class ListNode {
        size_t size;
        ListNode* next;
        friend LinkedListAllocator;

    public:
        explicit ListNode(size_t size): size(size), next(nullptr) {}
        VirtualAddress start_addr() {
            return reinterpret_cast<VirtualAddress>(this);
        }
        VirtualAddress end_addr() {
            return start_addr() + size;
        }
    };

    class LinkedListAllocator: public Allocator {
        ListNode head;

    public:
        LinkedListAllocator(): head(ListNode(0)) {}
        void init(size_t heap_start, size_t heap_size);
        void *allocate(size_t size, size_t align) override;
        void deallocate(void *ptr, size_t size) override;

    private:
        struct FindResult {ListNode *node; VirtualAddress start_addr;};
        void add_free_region(size_t addr, size_t size);
        rnt::Optional<FindResult> find_region(size_t size, size_t align);
    };

}

#endif //MAIN_LINKEDLISTALLOCATOR_H