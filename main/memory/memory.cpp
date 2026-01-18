#include "memory.h"

#include "bootinfo.hpp"
#include "frame_allocator.h"
#include "gdt.hpp"
#include "idt.hpp"
#include "paging/paging.h"
#include "virtual/BlockAllocator.h"
#include "x86/regs.h"

// Forward declarations from main.cpp
extern "C" void setup_idt_handlers();
extern "C" void kernel_main_continuation() __attribute__((noreturn));

namespace memory {

    static BlockAllocator(kernel_heap_obj);
    Allocator* kernel_heap = &kernel_heap_obj;

    alignas(AreaFrameAllocator) static uint8_t frame_allocator_storage[sizeof(AreaFrameAllocator)];
    FrameAllocator *frame_allocator = nullptr;

    // Store GDT/IDT pointers for use after jump
    static GDT* gdt_ptr = nullptr;
    static IDT* idt_ptr = nullptr;

    // Forward declaration - this runs at high addresses
    static void init_high_half() __attribute__((noreturn));

    void init(BootInfo& boot_info, GDT& gdt, IDT& idt) {
        using vga::out;

        // enable NO-EXECUTE bit for pages
        efer::enable_nxe_bit();
        // enable WRITE project bit for pages
        cr0::enable_write_protect();

        auto temp_var = AreaFrameAllocator::from_boot_info(boot_info);
        frame_allocator = new (frame_allocator_storage) AreaFrameAllocator(rnt::move(temp_var));

        // Store GDT/IDT pointers for use after jump
        gdt_ptr = &gdt;
        idt_ptr = &idt;

        // Step 1: Remap kernel with double mapping (low + high)
        paging::remap_the_kernel(*frame_allocator, boot_info);

        // Step 2: Jump to higher-half addresses and continue execution there
        // Note: This function does NOT return!
        paging::jump_to_higher_half(init_high_half);
    }

    // This function runs at high addresses only
    static void init_high_half() {
        using vga::out;

        // First output - test if we can even reach here
        out << ">>> ENTERED init_high_half <<<" << out.endl;
        out << "Now running at higher-half addresses! Proof: " << hex << (uint64_t) &init_high_half << out.endl;

        // Step 2.5: Update VGA buffer pointer to high address
        out.update_buffer_address(0xb8000 + paging::KERNEL_OFFSET);
        out << "VGA buffer updated to high address" << out.endl;

        // Step 2.6: Update GDT/IDT pointers to high addresses
        out << "Updating GDT/IDT pointers to high addresses..." << out.endl;
        gdt_ptr = reinterpret_cast<GDT*>(reinterpret_cast<uint64_t>(gdt_ptr) + paging::KERNEL_OFFSET);
        idt_ptr = reinterpret_cast<IDT*>(reinterpret_cast<uint64_t>(idt_ptr) + paging::KERNEL_OFFSET);
        GDT& gdt = *gdt_ptr;
        IDT& idt = *idt_ptr;
        out << "GDT/IDT pointers updated!" << out.endl;

        // Step 3: Rebuild GDT/IDT with high addresses
        out << "Rebuilding GDT/IDT with high addresses..." << out.endl;

        // Debug: Check what address setup_idt_handlers is at
        out << "setup_idt_handlers address: " << hex << reinterpret_cast<uint64_t>(&setup_idt_handlers) << out.endl;

        gdt.init();
        setup_idt_handlers();
        idt.load();
        out << "GDT and IDT rebuilt successfully!" << out.endl;

        // Step 3.5: Update data structure pointers to use high addresses
        out << "Updating frame allocator pointers to high addresses..." << out.endl;
        static_cast<AreaFrameAllocator*>(frame_allocator)->update_pointers_to_high(paging::KERNEL_OFFSET);
        out << "Frame allocator pointers updated!" << out.endl;

        // Step 4: Unmap lower-half identity mapping
        // TEMPORARILY DISABLED FOR TESTING
        // paging::unmap_lower_half(*frame_allocator);

        // Step 5: Map heap at high addresses
        auto page_table = paging::ActivePageTable::instance();
        auto heap_start_page = paging::Page::containing_address(HEAP_START);
        auto heap_end_page = paging::Page::containing_address(HEAP_START + HEAP_SIZE);
        for (auto i = heap_start_page.number; i <= heap_end_page.number; i++) {
            page_table.map(paging::Page(i), paging::PageFlags{.writable = true}, *frame_allocator);
        }

        kernel_heap_obj.init(HEAP_START, HEAP_SIZE);

        out << "Memory initialization complete! Kernel running at high addresses only." << out.endl;

        // Return control to the main kernel continuation
        kernel_main_continuation();
    }
}

