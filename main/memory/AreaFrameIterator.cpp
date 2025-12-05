//
// Created by Johannes Zottele on 04.12.25.
//

#include "AreaFrameIterator.h"
#include "bootinfo.hpp"
#include "frame.h"

AreaFrameIterator::AreaFrameIterator(
    const MemoryArea* area,
    PhysicalAddress kernel_start,
    PhysicalAddress kernel_end,
    PhysicalAddress multiboot_start,
    PhysicalAddress multiboot_end
)
    : current_frame(area->addr / memory::PAGE_SIZE)
    , end_frame((area->addr + area->len) / memory::PAGE_SIZE)
    , kernel_start_frame(kernel_start / memory::PAGE_SIZE)
    , kernel_end_frame((kernel_end + memory::PAGE_SIZE - 1) / memory::PAGE_SIZE)
    , multiboot_start_frame(multiboot_start / memory::PAGE_SIZE)
    , multiboot_end_frame((multiboot_end + memory::PAGE_SIZE - 1) / memory::PAGE_SIZE)
{
    // Align start frame up if not aligned
    if (area->addr % memory::PAGE_SIZE != 0) {
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

memory::Frame AreaFrameIterator::next() {
    if (!has_next()) {
        return memory::Frame(0);  // Invalid frame
    }

    memory::Frame frame(current_frame);
    current_frame++;

    // Skip reserved frames
    while (current_frame < end_frame && is_frame_reserved(current_frame)) {
        current_frame++;
    }

    return frame;
}
