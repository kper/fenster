#include "memory.h"

#include "bootinfo.hpp"
#include "frame_allocator.h"
#include "paging/paging.h"
#include "virtual/BlockAllocator.h"
#include "x86/regs.h"

// Forward declaration from main.cpp
extern "C" void kernel_main_high() __attribute__((noreturn));

namespace memory {

    static BlockAllocator(kernel_heap_obj);
    BlockAllocator* kernel_heap = &kernel_heap_obj;

    alignas(AreaFrameAllocator) static uint8_t frame_allocator_storage[sizeof(AreaFrameAllocator)];
    FrameAllocator *frame_allocator = nullptr;

    void init_and_jump_high(BootInfo& boot_info) {
        auto& out = vga::out();

        // Enable NO-EXECUTE bit for pages
        efer::enable_nxe_bit();
        // Enable WRITE protect bit for pages
        cr0::enable_write_protect();

        auto temp_var = AreaFrameAllocator::from_boot_info(boot_info);
        frame_allocator = new (frame_allocator_storage) AreaFrameAllocator(rnt::move(temp_var));

        // Remap kernel with double mapping (low + high)
        paging::remap_the_kernel(*frame_allocator, boot_info);

        out << "Map the HEAP" << out.endl;
        // Map heap at high addresses (before jumping so frame_allocator still works)
        auto page_table = paging::ActivePageTable::instance();
        auto heap_start_page = paging::Page::containing_address(HEAP_START);
        auto heap_end_page = paging::Page::containing_address(HEAP_START + HEAP_SIZE);
        for (auto i = heap_start_page.number; i <= heap_end_page.number; i++) {
            page_table.map(paging::Page(i), paging::PageFlags{.writable = true}, *frame_allocator);
        }

        // Update frame allocator pointers to high addresses before unmapping
        out << "Updating frame allocator to use high addresses..." << out.endl;
        ((AreaFrameAllocator*)frame_allocator)->update_pointers_to_high(paging::KERNEL_OFFSET);
        frame_allocator = (AreaFrameAllocator*) ((uint64_t)frame_allocator + paging::KERNEL_OFFSET);

        out << "Frame allocator updated!" << out.endl;

        out << "Jumping to high addresses..." << out.endl;
        // Jump to higher-half addresses and continue at kernel_main_high()
        // This function does NOT return!
        paging::jump_to_higher_half(kernel_main_high);
    }

    void init_heap() {
        auto& out = vga::out();
        // Initialize heap at high addresses (called after jumping to high half)
        out << "Initializing heap..." << out.endl;

        // No need for placement new anymore - no vtables!
        kernel_heap_obj.init(HEAP_START, HEAP_SIZE);
        kernel_heap = &kernel_heap_obj;

        out << "Heap initialized at " << hex << (uint64_t)kernel_heap << out.endl;
    }
}

