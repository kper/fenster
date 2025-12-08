//
// Created by Johannes Zottele on 08.12.25.
//

#include "LinkedListAllocator.h"

namespace memory {

    rnt::Optional<size_t> alloc_from_region(ListNode &region, size_t size, size_t align);

    struct Layout { size_t size; size_t alignment; };
    Layout size_align(size_t size, size_t align);


    void LinkedListAllocator::init(size_t heap_start, size_t heap_size) {
        add_free_region(heap_start, heap_size);
    }

    void *LinkedListAllocator::allocate(size_t size, size_t align) {
        auto layout = size_align(size, align);

        auto find_result = find_region(layout.size, layout.alignment);
        if (find_result.is_empty()) {
            return nullptr;
        }
        auto [region, alloc_start] = find_result.value();
        auto alloc_end = alloc_start + layout.size;
        auto remaining_size = region->end_addr() - alloc_end;
        if (remaining_size > 0) {
            add_free_region(alloc_end, remaining_size);
        }
        return reinterpret_cast<void*>(alloc_start);
    }

    void LinkedListAllocator::deallocate(void *ptr, size_t size) {
        // TODO: Make thread safe
        auto [adj_size, _] = size_align(size, 1);
        add_free_region(reinterpret_cast<size_t>(ptr), adj_size);
    }

    void LinkedListAllocator::add_free_region(const size_t addr, const size_t size) {
        ASSERT(align_up(addr, alignof(ListNode)) == addr, "Region must be ListNode aligned");
        ASSERT(size >= sizeof(ListNode), "Size too small to hold ListNode");

        const auto addr_ptr = reinterpret_cast<ListNode*>(addr);
        *addr_ptr = ListNode(size);
        addr_ptr->next = head.next;
        head.next = addr_ptr;
    }

    rnt::Optional<LinkedListAllocator::FindResult> LinkedListAllocator::find_region(size_t size, size_t align) {
        auto& current = head;

        while (current.next) {
            auto region = current.next;
            auto alloc_from_size = alloc_from_region(*region, size, align);
            if (alloc_from_size.is_empty()) {
                // region not suitable, check next one
                current = *region;
                continue;
            }

            // we found a suitable region, remove it from the current free list
            auto next = region->next;
            current.next = next;
            // return found region
            auto alloc_start = alloc_from_size.value();
            return FindResult{ .node = region, .start_addr = alloc_start };
        }

        return rnt::Optional<FindResult>();
    }

    rnt::Optional<size_t> alloc_from_region(ListNode &region, size_t size, size_t align) {
        auto alloc_start = align_up(region.start_addr(), align);
        auto alloc_end = saturating_add(alloc_start, size);

        if (alloc_end > region.end_addr()) {
            return rnt::Optional<size_t>();
        }

        auto remaining_size = region.end_addr() - alloc_end;
        if (remaining_size > 0 && remaining_size < sizeof(ListNode)) {
            // if the remaining size can't hold a ListNode, we can't use it, as we would lose
            // memory.
            return rnt::Optional<size_t>();
        }

        return alloc_start;
    }

    Layout size_align(size_t size, size_t align) {
        if (align < alignof(ListNode)) {
            // minimum alignment must be ListNode (everything bigger is already ListNode suitable)
            align = alignof(ListNode);
        }

        // align size up
        size = (size + align - 1) & ~(align - 1);

        if (size < sizeof(ListNode)) {
            size = sizeof(ListNode);
        }
        return {size, align};
    }


}