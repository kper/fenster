//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_PAGETABLE_H
#define MAIN_PAGETABLE_H

#include "PageTableEntry.h"
#include <stddef.h>

class VgaOutStream;

// Simple enable_if implementation for freestanding environment
template<bool B, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T> {
    using type = T;
};

// Forward declarations
template<int Level>
class PageTable;

// Type aliases for each level
using P4Table = PageTable<4>;  // PML4 (top level)
using P3Table = PageTable<3>;  // PDPT
using P2Table = PageTable<2>;  // PD
using P1Table = PageTable<1>;  // PT (leaf level)

/**
 * x86-64 Page Table
 * Template parameter Level indicates the table level (4, 3, 2, or 1)
 * Level 4 = PML4 (top level)
 * Level 3 = PDPT
 * Level 2 = PD
 * Level 1 = PT (leaf level, maps to actual pages)
 */
template<int Level>
class PageTable {
    static_assert(Level >= 1 && Level <= 4, "Page table level must be between 1 and 4");

private:
    PageTableEntry entries[512];

public:
    static constexpr uint16_t ENTRY_COUNT = 512;
    static constexpr int TABLE_LEVEL = Level;

    PageTable() {
        for (uint16_t i = 0; i < ENTRY_COUNT; i++) {
            entries[i].clear();
        }
    }

    // Array access
    PageTableEntry& operator[](uint16_t index) {
        return entries[index];
    }

    const PageTableEntry& operator[](uint16_t index) const {
        return entries[index];
    }

    // Direct entry access
    PageTableEntry* get_entries() {
        return entries;
    }

    const PageTableEntry* get_entries() const {
        return entries;
    }

    // Clear all entries
    void clear() {
        for (uint16_t i = 0; i < ENTRY_COUNT; i++) {
            entries[i].clear();
        }
    }

    // Get physical address of this table
    uint64_t get_physical_address() const {
        return reinterpret_cast<uint64_t>(this);
    }

    // Get next level table - only available for P4, P3, P2
    // P1 doesn't have this method (compile error if you try to call it)
    template<int L = Level>
    typename enable_if<(L > 1), PageTable<L - 1>*>::type
    get_next_table(size_t index) {
        return entries[index].template get_next_table<PageTable<L - 1>>();
    }

    template<int L = Level>
    typename enable_if<(L > 1), const PageTable<L - 1>*>::type
    get_next_table(size_t index) const {
        return entries[index].template get_next_table<PageTable<L - 1>>();
    }

    // Print table entries
    // recursive_level: 0 = current level only, 1 = one level down, -1 = all levels
    void print(VgaOutStream& stream, int recursive_level = 0, int indent = 0) const;

} __attribute__((aligned(4096)));

// Verify size for each instantiation
static_assert(sizeof(P4Table) == 4096, "P4Table must be 4096 bytes");
static_assert(sizeof(P3Table) == 4096, "P3Table must be 4096 bytes");
static_assert(sizeof(P2Table) == 4096, "P2Table must be 4096 bytes");
static_assert(sizeof(P1Table) == 4096, "P1Table must be 4096 bytes");

#endif //MAIN_PAGETABLE_H