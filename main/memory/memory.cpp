#include "memory.h"

#include "bootinfo.hpp"
#include "frame_allocator.h"
#include "paging/paging.h"
#include "virtual/BlockAllocator.h"
#include "x86/regs.h"


namespace memory {

    static BlockAllocator(kernel_heap_obj);
    Allocator* kernel_heap = &kernel_heap_obj;

    alignas(AreaFrameAllocator) static uint8_t frame_allocator_storage[sizeof(AreaFrameAllocator)];
    FrameAllocator *frame_allocator = nullptr;

    void init(BootInfo& boot_info) {
        // enable NO-EXECUTE bit for pages
        efer::enable_nxe_bit();
        // enable WRITE project bit for pages
        cr0::enable_write_protect();

        auto page_table = paging::ActivePageTable::instance();
        auto temp_var = AreaFrameAllocator::from_boot_info(boot_info);
        frame_allocator = new (frame_allocator_storage) AreaFrameAllocator(rnt::move(temp_var));

        paging::remap_the_kernel(*frame_allocator, boot_info);

        auto heap_start_page = paging::Page::containing_address(HEAP_START);
        auto heap_end_page = paging::Page::containing_address(HEAP_START + HEAP_SIZE);
        for (auto i = heap_start_page.number; i <= heap_end_page.number; i++) {
            page_table.map(paging::Page(i), paging::PageFlags{.writable = true}, *frame_allocator);
        }

        kernel_heap_obj.init(HEAP_START, HEAP_SIZE);
    }
}

