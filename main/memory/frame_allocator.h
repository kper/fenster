//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_FRAME_H
#define MAIN_FRAME_H

#include <stdint.h>
#include "AreaFrameIterator.h"
#include "memory.h"

// Forward declarations
struct Multiboot2TagMmap;
struct MemoryArea;

/**
 * Abstract interface for frame allocators
 * All frame allocators must implement this interface
 */
class FrameAllocator {
public:
    virtual ~FrameAllocator() = default;
    virtual memory::Frame allocate_frame() = 0;
    virtual void deallocate_frame(memory::Frame frame) = 0;
};

/**
 * Frame allocator that uses memory areas from multiboot memory map
 * Allocates frames from available physical memory regions
 */
class AreaFrameAllocator : public FrameAllocator {
private:
    const Multiboot2TagMmap* mmap;
    const MemoryArea* current_area;
    AreaFrameIterator current_iterator;
    bool has_valid_iterator;
    PhysicalAddress kernel_start;
    PhysicalAddress kernel_end;
    PhysicalAddress multiboot_start;
    PhysicalAddress multiboot_end;

    /**
     * Find next available area and update current iterator
     */
    void advance_to_next_area();

public:
    /**
     * Initialize frame allocator with memory map and kernel bounds
     * @param mmap Memory map from multiboot
     * @param kernel_start Physical start address of kernel
     * @param kernel_end Physical end address of kernel
     * @param multiboot_start Physical start address of multiboot info
     * @param multiboot_end Physical end address of multiboot info
     */
    AreaFrameAllocator(
        const Multiboot2TagMmap* mmap,
        PhysicalAddress kernel_start,
        PhysicalAddress kernel_end,
        PhysicalAddress multiboot_start,
        PhysicalAddress multiboot_end
    );

    /**
     * Allocate a single frame
     * @return Allocated frame, or Frame(0) if no memory available
     */
    memory::Frame allocate_frame() override;

    /**
     * Deallocate a frame (currently a no-op, future: bitmap allocator)
     * @param frame Frame to deallocate
     */
    void deallocate_frame(memory::Frame frame) override;
};

#endif //MAIN_FRAME_H
