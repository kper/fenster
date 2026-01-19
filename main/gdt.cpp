#include "gdt.hpp"

#include "vga.hpp"
#include "paging/paging.h"
#include "memory/memory.h"
#include "Process.h"

void GDT::init()
{
    gdt[0] = 0;

    // Kernel code segment descriptor (ring 0):
    // 41: readable
    // 43: code/data segment
    // 44: descriptor type (user vs system)
    // 47: present
    // 53: 64-bit CS
    gdt[1] = (1ull << 41) | (1ull << 43) | (1ull << 44) | (1ull << 47) | (1ull << 53);

    // Kernel data segment descriptor (ring 0):
    // 41: writable
    // 44: descriptor type
    // 47: present
    gdt[2] = (1ull << 41) | (1ull << 44) | (1ull << 47);

    // User code segment (ring 3):
    // 41: readable, 43: code/data, 44: descriptor type, 45-46: DPL=3, 47: present, 53: 64-bit
    gdt[5] = (1ull << 41) | (1ull << 43) | (1ull << 44) | (3ull << 45) | (1ull << 47) | (1ull << 53);

    // User data segment (ring 3):
    // 41: writable, 44: descriptor type, 45-46: DPL=3, 47: present
    gdt[6] = (1ull << 41) | (1ull << 44) | (3ull << 45) | (1ull << 47);

    uint64_t dfs_bot = reinterpret_cast<uint64_t>(&df_stack[0]);
    uint64_t dfs_top = dfs_bot + DF_SIZE;

    // Prepare the TSS:
    tss.reserved0 = 0;
    tss.reserved1 = 0;
    tss.reserved2 = 0;
    tss.reserved3 = 0;
    tss.io = sizeof(tss);

    // Set up kernel stack for when we return from ring 3 (syscalls, interrupts)
    tss.pst.s0 = reinterpret_cast<uint64_t>(&kernel_stack[0]) + sizeof(kernel_stack);

    // Assign the #DF stack at IST idx 1
    tss.ist.s0 = dfs_top;

    // TSS descriptor (system descriptor, spans 2 GDT entries at gdt[3..4])
    gdt[3] = 0; // zero out
    gdt[4] = 0;

    uint16_t tss_limit = static_cast<uint16_t>(sizeof(tss) - 1);
    uint64_t tss_base  = reinterpret_cast<uint64_t>(&tss);

    gdt[3] |= tss_limit;                    // the tss limit
    gdt[3] |= (tss_base & 0xFFFFFF) << 16;  // First 3 bytes of the TSS base address
    gdt[3] |= 0b1001ull << 40;              // type (64-bit TSS)
    gdt[3] |= ((uint64_t) 1) << 47;         // present bit
    gdt[3] |= (0xFF & (tss_base >> 24)) << 56; // 4th byte of the address
    gdt[4] |= tss_base >> 32;               // The last 4 bytes of the address

    // Load the GDT
    GdtDescriptor d = {
        static_cast<uint16_t>(sizeof(gdt) - 1),
        reinterpret_cast<uint64_t>(&gdt[0])
    };
    asm volatile("lgdt %0" : : "m"(d) : "memory");

    // Load the CS (far ret to the next insn)
    asm volatile(
        "pushq %[cs]\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        :
        : [cs] "i"(CS_SELECTOR)
        : "rax", "memory"
    );

    // Load the TSS
    asm volatile ("ltr %0" :: "r"((uint16_t)TSS_SELECTOR));
}

void GDT::jump_to_ring3(void (*user_function)())
{
    auto out = vga::out();

    // User space memory layout:
    // 0xB0000000 - 0xB0800000: 8MB heap (grows upward)
    // 0xBE000000 - 0xC0000000: 2MB stack (grows downward from 0xC0000000)

    uint64_t USER_HEAP_START = 0xB0000000;
    uint64_t USER_HEAP_SIZE = 8 * 1024 * 1024;   // 8MB heap
    uint64_t USER_STACK_TOP = 0xC0000000;
    uint64_t USER_STACK_SIZE = 2 * 1024 * 1024;  // 2MB stack

    auto page_table = paging::ActivePageTable::instance();

    out << "User function addr: " << (void *)user_function << out.endl;
    out << "Setting up user heap: " << (void*)USER_HEAP_START << " - " << (void*)(USER_HEAP_START + USER_HEAP_SIZE) << out.endl;
    out << "Setting up user stack: " << (void*)(USER_STACK_TOP - USER_STACK_SIZE) << " - " << (void*)USER_STACK_TOP << out.endl;

    // Map user heap with user_accessible and writable flags
    auto heap_start_page = paging::Page::containing_address(USER_HEAP_START);
    auto heap_end_page = paging::Page::containing_address(USER_HEAP_START + USER_HEAP_SIZE - 1);
    for (auto i = heap_start_page.number; i <= heap_end_page.number; i++) {
        page_table.map(paging::Page(i), paging::PageFlags{.writable = true, .user_accessible = true}, *memory::frame_allocator);
    }

    // Map user stack with user_accessible and writable flags
    auto stack_top_page = paging::Page::containing_address(USER_STACK_TOP - USER_STACK_SIZE);
    auto stack_bottom_page = paging::Page::containing_address(USER_STACK_TOP);
    for (auto i = stack_bottom_page.number; i > stack_top_page.number; i--) {
        page_table.map(paging::Page(i), paging::PageFlags{.writable = true, .user_accessible = true}, *memory::frame_allocator);
    }

    // Map the user function code page as user-accessible
    // Note: This maps kernel code to be user-accessible, which is a security risk
    // In a real OS, you'd copy the code to user space instead
    auto user_func_addr = reinterpret_cast<uint64_t>(user_function);
    auto user_func_page = paging::Page::containing_address(user_func_addr);

    out << "Mapping user function page: " << (void*)user_func_page.start_addr() << out.endl;

    // We need to set the user-accessible flag on all levels of the page table hierarchy
    // for the user function's page. We do this by walking the page table via recursive mapping.
    auto p4 = reinterpret_cast<paging::P4Table*>(paging::RECURSIVE_P4_ADDR);
    uint16_t p4_index = (user_func_addr >> 39) & 0x1FF;
    uint16_t p3_index = (user_func_addr >> 30) & 0x1FF;
    uint16_t p2_index = (user_func_addr >> 21) & 0x1FF;
    uint16_t p1_index = (user_func_addr >> 12) & 0x1FF;

    // Set user-accessible on P4 entry
    (*p4)[p4_index].set_user_accessible(true);

    // Get P3 table and set user-accessible
    auto p3 = p4->get_next_table(p4_index);
    if (p3) {
        (*p3)[p3_index].set_user_accessible(true);

        // Get P2 table and set user-accessible
        auto p2 = p3->get_next_table(p3_index);
        if (p2) {
            (*p2)[p2_index].set_user_accessible(true);

            // Get P1 table and set user-accessible
            auto p1 = p2->get_next_table(p2_index);
            if (p1) {
                (*p1)[p1_index].set_user_accessible(true);
                out << "Set user-accessible flag on user function page" << out.endl;
            }
        }
    }

    // Flush TLB for the modified page
    asm volatile("invlpg (%0)" :: "r"(user_func_addr) : "memory");

    // Create and initialize the process with its heap
    // Allocate Process object from kernel heap
    auto process = reinterpret_cast<Process*>(memory::kernel_heap->allocate(sizeof(Process), alignof(Process)));

    // Allocate BlockAllocator for the process heap from kernel heap
    process->heap = reinterpret_cast<memory::BlockAllocator*>(
        memory::kernel_heap->allocate(sizeof(memory::BlockAllocator), alignof(memory::BlockAllocator))
    );

    // Placement new to construct the BlockAllocator
    new (process->heap) memory::BlockAllocator();

    // Initialize the user heap with the memory range we just mapped
    process->heap->init(USER_HEAP_START, USER_HEAP_SIZE);

    // Set as active process so syscalls can access it
    process::activeProcess = process;

    out << "Process created with heap at " << (void*)USER_HEAP_START << out.endl;

    // Get current RFLAGS and set interrupt flag
    uint64_t rflags;
    asm volatile("pushfq; pop %0" : "=r"(rflags));
    rflags |= 0x200; // Set IF (interrupt enable flag)

    // Jump to ring 3 using iretq
    // Stack layout for iretq: SS, RSP, RFLAGS, CS, RIP
    asm volatile(
        "pushq %[ss]\n"          // Push user data segment selector
        "pushq %[rsp]\n"         // Push user stack pointer
        "pushq %[rflags]\n"      // Push RFLAGS
        "pushq %[cs]\n"          // Push user code segment selector
        "pushq %[rip]\n"         // Push instruction pointer (user function)
        "iretq\n"                // Return to ring 3
        :
        : [ss] "r"((uint64_t)USER_DATA_SELECTOR),
          [rsp] "r"(USER_STACK_TOP),
          [rflags] "r"(rflags),
          [cs] "r"((uint64_t)USER_CODE_SELECTOR),
          [rip] "r"((uint64_t)user_function)
        : "memory"
    );
}
