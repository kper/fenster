//
// Created by Johannes Zottele on 04.12.25.
//

#include "frame_allocator.h"
#include "AreaFrameIterator.h"
#include "vga.hpp"
#include "../bootinfo.hpp"
#include "virtual/VirtualAllocator.h"

namespace memory {

    extern Allocator* kernel_heap;

    void FreeList::init(Frame *backing_buffer, size_t capacity) {
        data_ = backing_buffer;
        capacity_ = capacity;
        size_ = 0;
        heap_owner_ = nullptr;
        uses_heap_storage_ = false;
    }

    bool FreeList::try_grow(Allocator *heap) {
        if (heap == nullptr) {
            return false;
        }

        size_t new_capacity = capacity_ * 2;
        size_t bytes = new_capacity * sizeof(Frame);
        void* mem = heap->allocate(bytes, alignof(Frame));
        if (!mem) {
            return false;
        }

        Frame* new_data = reinterpret_cast<Frame*>(mem);
        for (size_t i = 0; i < size_; i++) {
            new_data[i] = data_[i];
        }

        if (uses_heap_storage_ && heap_owner_) {
            heap_owner_->deallocate(data_, capacity_ * sizeof(Frame));
        }

        data_ = new_data;
        capacity_ = new_capacity;
        heap_owner_ = heap;
        uses_heap_storage_ = true;
        return true;
    }

    bool FreeList::push(Frame frame, Allocator *heap) {
        if (size_ >= capacity_) {
            if (!try_grow(heap)) {
                return false;
            }
        }
        data_[size_++] = frame;
        return true;
    }

    rnt::Optional<Frame> FreeList::pop() {
        if (size_ == 0) {
            return rnt::Optional<Frame>();
        }
        size_--;
        return data_[size_];
    }

    AreaFrameAllocator::AreaFrameAllocator(
        const Multiboot2TagMmap* mmap,
        PhysicalAddress kernel_start,
        PhysicalAddress kernel_end,
        PhysicalAddress multiboot_start,
        PhysicalAddress multiboot_end
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
        free_list.init(free_list_storage, INITIAL_FREE_CAPACITY);
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

    AreaFrameAllocator AreaFrameAllocator::from_boot_info(const BootInfo &boot_info) {
        uint64_t kernel_start = UINT64_MAX;
        uint64_t kernel_end = 0;
        auto elf_sections = boot_info.get_elf_sections();
        if (elf_sections.has_value()) {
            const auto elf = elf_sections.value();

            for (auto section = elf->sections_begin(); section != elf->sections_end(); ++section) {
                if (section->size == 0) continue;
                if (section->addr < kernel_start) kernel_start = section->addr;
                if (section->addr + section->size > kernel_end) kernel_end = section->addr + section->size;
            }
        }

        using vga::out;
        out << "Kernel Start: " << hex << kernel_start << out.endl;
        out << "Kernel End: " << hex << kernel_end << out.endl;
        out << "Kernel Size: " << size << kernel_end << out.endl;

        uint64_t multiboot_start = reinterpret_cast<uint64_t>(&boot_info);
        uint64_t multiboot_end   = multiboot_start + boot_info.get_total_size();
        return AreaFrameAllocator(boot_info.get_memory_map(), kernel_start, kernel_end, multiboot_start, multiboot_end);
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

    rnt::Optional<Frame> AreaFrameAllocator::allocate_frame() {
        // Reuse a freed frame if available.
        auto reused = free_list.pop();
        if (reused.has_value()) {
            return reused;
        }

        // Keep trying until we find a frame or run out of areas
        while (has_valid_iterator) {
            if (current_iterator.has_next()) {
                return current_iterator.next();
            }

            // Current area exhausted, move to next
            advance_to_next_area();
        }

        // Out of memory
        return rnt::Optional<Frame>();
    }

    void AreaFrameAllocator::deallocate_frame(memory::Frame frame) {
        // Store the frame for later reuse. Grow using the heap if available.
        ASSERT(free_list.push(frame, kernel_heap), "Frame free list exhausted");
    }

    void AreaFrameAllocator::update_pointers_to_high(uint64_t offset) {
        // Update mmap pointer to high address
        if (mmap != nullptr) {
            mmap = reinterpret_cast<const Multiboot2TagMmap*>(
                reinterpret_cast<uint64_t>(mmap) + offset
            );
        }

        // Update current_area pointer if it's valid
        if (current_area != nullptr) {
            current_area = reinterpret_cast<const MemoryArea*>(
                reinterpret_cast<uint64_t>(current_area) + offset
            );
        }

        // Note: AreaFrameIterator doesn't store pointers, only frame numbers,
        // so it doesn't need updating
        // Note: FreeList uses static storage initially, which is part of this object,
        // so it will be accessible at high addresses once we're running there
    }
}
