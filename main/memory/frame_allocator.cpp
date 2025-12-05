//
// Created by Johannes Zottele on 04.12.25.
//

#include "frame_allocator.h"
#include "AreaFrameIterator.h"
#include "vga.hpp"
#include "../bootinfo.hpp"

namespace memory {
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
        // TODO: Implement deallocation with bitmap
        // For now, this is a no-op as we have a bump allocator
        (void)frame;  // Suppress unused parameter warning
    }
}