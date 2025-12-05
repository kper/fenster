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
    inline paging::P4Table* get_virt_p4_table() {
        // With P4[511] pointing to P4 itself, we can access P4 at [511,511,511,511]
        return reinterpret_cast<paging::P4Table*>(paging::RECURSIVE_P4_ADDR);
    }

    inline PhysicalAddress get_phys_addr() {
        uint64_t cr3_value;
        asm volatile("mov %%cr3, %0" : "=r"(cr3_value));
        // CR3 bits 12-51 contain the physical address of P4 table
        return cr3_value & 0x000FFFFFFFFFF000ULL;
    }

    inline memory::Frame get_frame() {
        return memory::Frame::containing_address(get_phys_addr());
    }

    /**
     * Set the physical address to a new P4 Table
     * This will flush the TLB (Translation Lookaside Buffer)
     * @param phys_addr Physical address of the new P4 table
     */
    inline void set_phys_addr(PhysicalAddress phys_addr) {
        uint64_t addr = phys_addr;
        // Ensure the address is page-aligned
        addr &= 0x000FFFFFFFFFF000ULL;
        asm volatile("mov %0, %%cr3" : : "r"(addr) : "memory");
    }

    /**
     * Reload CR3 to flush the TLB
     */
    inline void flush() {
        set_phys_addr(get_phys_addr());
    }
}

#endif //MAIN_C3_H