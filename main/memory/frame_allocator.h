//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_FRAME_H
#define MAIN_FRAME_H

#include "AreaFrameIterator.h"
#include "bootinfo.hpp"
#include "runtime/optional.h"
#include <stddef.h>

// Forward declarations
struct Multiboot2TagMmap;
struct MemoryArea;

namespace memory {

    class Allocator;

    /**
     * Simple LIFO free list for frames.
     * Backed by a small static buffer and can grow using the kernel heap once available.
     */
    class FreeList {
    public:
        void init(Frame* backing_buffer, size_t capacity);
        bool push(Frame frame, Allocator* heap);
        rnt::Optional<Frame> pop();
        bool empty() const { return size_ == 0; }

    private:
        Frame*   data_ = nullptr;
        size_t   capacity_ = 0;
        size_t   size_ = 0;
        Allocator* heap_owner_ = nullptr;
        bool     uses_heap_storage_ = false;

        bool try_grow(Allocator* heap);
    };

    /**
     * Abstract interface for frame allocators
     * All frame allocators must implement this interface
     */
    class FrameAllocator {
    public:
        virtual rnt::Optional<Frame> allocate_frame() = 0;
        virtual void deallocate_frame(Frame frame) = 0;
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
        static constexpr size_t INITIAL_FREE_CAPACITY = 128;
        FreeList free_list;
        Frame free_list_storage[INITIAL_FREE_CAPACITY];

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

        static AreaFrameAllocator from_boot_info(
            const BootInfo& boot_info
        );

        /**
         * Allocate a single frame
         */
        rnt::Optional<Frame> allocate_frame() override;

        /**
         * Deallocate a frame (currently a no-op, future: bitmap allocator)
         * @param frame Frame to deallocate
         */
        void deallocate_frame(Frame frame) override;

        /**
         * Update internal pointers to use high addresses after jumping to higher-half
         * This is necessary because the allocator is created at low addresses before the jump
         * @param offset Offset to add to all pointers (typically KERNEL_OFFSET)
         */
        void update_pointers_to_high(uint64_t offset);
    };
}
#endif //MAIN_FRAME_H
