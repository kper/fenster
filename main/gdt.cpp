#include "gdt.hpp"

#include "vga.hpp"

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
    // Allocate user stack (static for simplicity)
    static uint8_t user_stack[8192];
    uint64_t user_stack_top = reinterpret_cast<uint64_t>(&user_stack[0]) + sizeof(user_stack);

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
          [rsp] "r"(user_stack_top),
          [rflags] "r"(rflags),
          [cs] "r"((uint64_t)USER_CODE_SELECTOR),
          [rip] "r"((uint64_t)user_function)
        : "memory"
    );
}
