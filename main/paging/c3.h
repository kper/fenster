//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_C3_H
#define MAIN_C3_H

#include "Table.h"

/**
 * CR3 (Control Register 3) management
 * CR3 holds the physical address of the P4 (PML4) page table
 */
namespace c3 {
    /**
     * Get the current P4 table from CR3
     * With recursive page tables, P4 is always accessible at the recursive address
     * @return Pointer to the current P4 table
     */
    inline paging::P4Table* get() {
        // With P4[511] pointing to P4 itself, we can access P4 at [511,511,511,511]
        return reinterpret_cast<paging::P4Table*>(paging::RECURSIVE_P4_ADDR);
    }

    /**
     * Set the P4 table in CR3
     * This will flush the TLB (Translation Lookaside Buffer)
     * @param table Pointer to the new P4 table
     */
    inline void set(paging::P4Table* table) {
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