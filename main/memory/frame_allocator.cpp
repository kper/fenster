//
// Created by Johannes Zottele on 04.12.25.
//

#include "frame_allocator.h"
#include "AreaFrameIterator.h"
#include "../bootinfo.hpp"

AreaFrameAllocator::AreaFrameAllocator(
    const Multiboot2TagMmap* mmap,
    uint64_t kernel_start,
    uint64_t kernel_end,
    uint64_t multiboot_start,
    uint64_t multiboot_end
)
    : mmap(mmap)
    , current_area(nullptr)
    , current_iterator()
    , has_valid_iterator(false)
    , kernel_start(kernel_start)
    , kernel_end(kernel_end)
    , multiboot_start(multiboot_start)
    , multiboot_end(multiboot_end)
{
    // Find first available area and create iterator
    for (auto area = mmap->entries_begin(); area != mmap->entries_end(); ++area) {
        if (area->is_available()) {
            current_area = area;
            current_iterator = AreaFrameIterator(
                area,
                kernel_start,
                kernel_end,
                multiboot_start,
                multiboot_end
            );
            has_valid_iterator = true;
            break;
        }
    }
}

void AreaFrameAllocator::advance_to_next_area() {
    if (current_area == nullptr) {
        return;
    }

    has_valid_iterator = false;

    // Move to next area
    current_area++;

    // Find next available area
    while (current_area < mmap->entries_end()) {
        if (current_area->is_available()) {
            current_iterator = AreaFrameIterator(
                current_area,
                kernel_start,
                kernel_end,
                multiboot_start,
                multiboot_end
            );
            has_valid_iterator = true;
            return;
        }
        current_area++;
    }

    // No more areas
    current_area = nullptr;
}

Frame AreaFrameAllocator::allocate_frame() {
    // Keep trying until we find a frame or run out of areas
    while (has_valid_iterator) {
        if (current_iterator.has_next()) {
            return current_iterator.next();
        }

        // Current area exhausted, move to next
        advance_to_next_area();
    }

    // Out of memory
    return Frame(0);
}

void AreaFrameAllocator::deallocate_frame(Frame frame) {
    // TODO: Implement deallocation with bitmap
    // For now, this is a no-op as we have a bump allocator
    (void)frame;  // Suppress unused parameter warning
}
