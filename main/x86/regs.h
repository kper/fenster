//
// Created by Johannes Zottele on 05.12.25.
//

#ifndef MAIN_REGS_H
#define MAIN_REGS_H
#include <stdint.h>

namespace msr {
    // Model-Specific Register addresses
    constexpr uint32_t IA32_EFER = 0xC0000080;

    // Read MSR
    inline uint64_t read(uint32_t msr) {
        uint32_t eax, edx;
        asm volatile(
            "rdmsr"
            : "=a"(eax), "=d"(edx)
            : "c"(msr)
        );
        return (static_cast<uint64_t>(edx) << 32) | eax;
    }

    // Write MSR
    inline void write(uint32_t msr, uint64_t value) {
        uint32_t eax = value & 0xFFFFFFFF;
        uint32_t edx = value >> 32;
        asm volatile(
            "wrmsr"
            :
            : "a"(eax), "d"(edx), "c"(msr)
        );
    }
}

namespace efer {
    // EFER (Extended Feature Enable Register) bits
    constexpr uint64_t NXE = 1ULL << 11;  // No-Execute Enable

    inline uint64_t read() {
        return msr::read(msr::IA32_EFER);
    }

    inline void write(uint64_t value) {
        msr::write(msr::IA32_EFER, value);
    }

    inline void enable_nxe_bit() {
        uint64_t efer = read();
        efer |= NXE;
        write(efer);
    }
}

namespace cr0 {
    // CR0 control register bits
    constexpr uint64_t WP = 1ULL << 16;  // Write Protect

    inline uint64_t read() {
        uint64_t value;
        asm volatile("mov %%cr0, %0" : "=r"(value));
        return value;
    }

    inline void write(uint64_t value) {
        asm volatile("mov %0, %%cr0" : : "r"(value));
    }

    inline void enable_write_protect() {
        uint64_t cr0 = read();
        cr0 |= WP;
        write(cr0);
    }
}

namespace cr2 {
    // Returns the Page Fault Linear Address
    inline uint64_t get_pfla() {
        uint64_t cr2_value;
        asm volatile("mov %%cr2, %0" : "=r"(cr2_value));
        return cr2_value;
    }
}

#endif //MAIN_REGS_H