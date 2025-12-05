//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_AREA_FRAME_ITERATOR_H
#define MAIN_AREA_FRAME_ITERATOR_H

#include <stdint.h>
#include "frame.h"

// Forward declarations
struct MemoryArea;

/**
 * Iterator for frames within a memory area, excluding kernel and multiboot regions
 */
class AreaFrameIterator {
private:
    uint64_t current_frame;
    uint64_t end_frame;
    uint64_t kernel_start_frame;
    uint64_t kernel_end_frame;
    uint64_t multiboot_start_frame;
    uint64_t multiboot_end_frame;

    bool is_frame_reserved(uint64_t frame_num) const;

public:
    /**
     * Default constructor - creates invalid iterator
     */
    AreaFrameIterator()
        : current_frame(0)
        , end_frame(0)
        , kernel_start_frame(0)
        , kernel_end_frame(0)
        , multiboot_start_frame(0)
        , multiboot_end_frame(0)
    {}

    /**
     * Create iterator for a memory area
     * @param area Memory area to iterate over
     * @param kernel_start Physical address of kernel start
     * @param kernel_end Physical address of kernel end
     * @param multiboot_start Physical address of multiboot info start
     * @param multiboot_end Physical address of multiboot info end
     */
    AreaFrameIterator(
        const MemoryArea* area,
        uint64_t kernel_start,
        uint64_t kernel_end,
        uint64_t multiboot_start,
        uint64_t multiboot_end
    );

    bool has_next() const;

    memory::Frame next();
};

#endif //MAIN_AREA_FRAME_ITERATOR_H
