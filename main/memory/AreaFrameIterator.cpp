//
// Created by Johannes Zottele on 04.12.25.
//

#include "AreaFrameIterator.h"
#include "../bootinfo.hpp"

AreaFrameIterator::AreaFrameIterator(
    const MemoryArea* area,
    uint64_t kernel_start,
    uint64_t kernel_end,
    uint64_t multiboot_start,
    uint64_t multiboot_end
)
    : current_frame(area->addr / PAGE_SIZE)
    , end_frame((area->addr + area->len) / PAGE_SIZE)
    , kernel_start_frame(kernel_start / PAGE_SIZE)
    , kernel_end_frame((kernel_end + PAGE_SIZE - 1) / PAGE_SIZE)
    , multiboot_start_frame(multiboot_start / PAGE_SIZE)
    , multiboot_end_frame((multiboot_end + PAGE_SIZE - 1) / PAGE_SIZE)
{
    // Align start frame up if not aligned
    if (area->addr % PAGE_SIZE != 0) {
        current_frame++;
    }

    // Skip to first non-reserved frame
    while (current_frame < end_frame && is_frame_reserved(current_frame)) {
        current_frame++;
    }
}

bool AreaFrameIterator::is_frame_reserved(uint64_t frame_num) const {
    // Check if frame is in kernel range
    if (frame_num >= kernel_start_frame && frame_num < kernel_end_frame) {
        return true;
    }

    // Check if frame is in multiboot range
    if (frame_num >= multiboot_start_frame && frame_num < multiboot_end_frame) {
        return true;
    }

    return false;
}

bool AreaFrameIterator::has_next() const {
    return current_frame < end_frame;
}

Frame AreaFrameIterator::next() {
    if (!has_next()) {
        return Frame(0);  // Invalid frame
    }

    Frame frame(current_frame);
    current_frame++;

    // Skip reserved frames
    while (current_frame < end_frame && is_frame_reserved(current_frame)) {
        current_frame++;
    }

    return frame;
}
