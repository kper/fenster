//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_C3_H
#define MAIN_C3_H

#include "PageTable.h"

/**
 * CR3 (Control Register 3) management
 * CR3 holds the physical address of the P4 (PML4) page table
 */
namespace c3 {
    /**
     * Get the current P4 table from CR3
     * @return Pointer to the current P4 table
     */
    inline P4Table* get() {
        uint64_t cr3_value;
        asm volatile("mov %%cr3, %0" : "=r"(cr3_value));
        // CR3 bits 12-51 contain the physical address of P4 table
        return reinterpret_cast<P4Table*>(cr3_value & 0x000FFFFFFFFFF000ULL);
    }

    /**
     * Set the P4 table in CR3
     * This will flush the TLB (Translation Lookaside Buffer)
     * @param table Pointer to the new P4 table
     */
    inline void set(P4Table* table) {
        uint64_t addr = reinterpret_cast<uint64_t>(table);
        // Ensure the address is page-aligned
        addr &= 0x000FFFFFFFFFF000ULL;
        asm volatile("mov %0, %%cr3" : : "r"(addr) : "memory");
    }

    /**
     * Reload CR3 to flush the TLB
     */
    inline void flush() {
        set(get());
    }
}

#endif //MAIN_C3_H