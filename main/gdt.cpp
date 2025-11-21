#include "gdt.hpp"

void GDT::init()
{
    gdt[0] = 0;

    // Code segment descriptor:
    // 41: readable
    // 43: code/data segment
    // 44: descriptor type (user vs system)
    // 47: present
    // 53: 64-bit CS
    gdt[1] = (1ull << 41) | (1ull << 43) | (1ull << 44) | (1ull << 47) | (1ull << 53);

    uint64_t dfs_bot = reinterpret_cast<uint64_t>(&df_stack[0]);
    uint64_t dfs_top = dfs_bot + DF_SIZE;

    // Prepare the TSS:
    tss.reserved0 = 0;
    tss.reserved1 = 0;
    tss.reserved2 = 0;
    tss.reserved3 = 0;
    tss.io = sizeof(tss);

    // Assign the #DF stack at IST idx 1
    tss.ist.s0 = dfs_top;

    // TSS descriptor (system descriptor, spans 2 GDT entries)
    gdt[2] = 0; // zero out
    gdt[3] = 0;

    uint16_t tss_limit = static_cast<uint16_t>(sizeof(tss) - 1);
    uint64_t tss_base  = reinterpret_cast<uint64_t>(&tss);

    gdt[2] |= tss_limit;                    // the tss limit
    gdt[2] |= (tss_base & 0xFFFFFF) << 16;  // First 3 bytes of the TSS base address
    gdt[2] |= 0b1001ull << 40;              // type (64-bit TSS)
    gdt[2] |= ((uint64_t) 1) << 47;         // present bit
    gdt[2] |= (0xFF & (tss_base >> 24)) << 56; // 4th byte of the address
    gdt[3] |= tss_base >> 32;               // The last 4 bytes of the address

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
