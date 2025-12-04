//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_AREA_FRAME_ITERATOR_H
#define MAIN_AREA_FRAME_ITERATOR_H

#include <stdint.h>

constexpr uint64_t PAGE_SIZE = 4096;

// Forward declaration
struct MemoryArea;

/**
 * Represents a physical memory frame (4KB page)
 */
class Frame {
public:
    uint64_t number;

    Frame() : number(0) {}
    explicit Frame(uint64_t num) : number(num) {}

    uint64_t start_address() const {
        return number * PAGE_SIZE;
    }

    static Frame containing_address(uint64_t addr) {
        return Frame(addr / PAGE_SIZE);
    }

    Frame clone() const {
        return Frame(number);
    }

    bool operator==(const Frame& other) const {
        return number == other.number;
    }

    bool operator!=(const Frame& other) const {
        return number != other.number;
    }
};

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
    Frame next();
};

#endif //MAIN_AREA_FRAME_ITERATOR_H
