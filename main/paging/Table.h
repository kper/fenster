//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_PAGETABLE_H
#define MAIN_PAGETABLE_H

#include "Entry.h"
#include <stddef.h>

using VirtualAddress = uint64_t;

class VgaOutStream;

namespace paging {

// Forward declaration
struct Frame;

// Recursive page table constants
// With P4[511] pointing to P4 itself, we can access all page tables through these addresses
constexpr uint64_t RECURSIVE_INDEX = 511;
constexpr uint64_t RECURSIVE_P4_ADDR = 0xFFFF'FFFF'FFFF'F000ULL;  // [511,511,511,511]
constexpr uint64_t RECURSIVE_P3_BASE = 0xFFFF'FFFF'FFE0'0000ULL;  // [511,511,511,X]
constexpr uint64_t RECURSIVE_P2_BASE = 0xFFFF'FFC0'0000'0000ULL;  // [511,511,X,Y]
constexpr uint64_t RECURSIVE_P1_BASE = 0xFFFF'8000'0000'0000ULL;  // [511,X,Y,Z]

// Simple enable_if implementation for freestanding environment
template<bool B, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T> {
    using type = T;
};

// Forward declarations
template<int Level>
class Table;

// Type aliases for each level
using P4Table = Table<4>;  // PML4 (top level)
using P3Table = Table<3>;  // PDPT
using P2Table = Table<2>;  // PD
using P1Table = Table<1>;  // PT (leaf level)

/**
 * x86-64 Page Table
 * Template parameter Level indicates the table level (4, 3, 2, or 1)
 * Level 4 = PML4 (top level)
 * Level 3 = PDPT
 * Level 2 = PD
 * Level 1 = PT (leaf level, maps to actual pages)
 */
template<int Level>
class Table {
    static_assert(Level >= 1 && Level <= 4, "Page table level must be between 1 and 4");

private:
    Entry entries[512];

public:
    static constexpr uint16_t ENTRY_COUNT = 512;
    static constexpr int TABLE_LEVEL = Level;

    Table() {
        for (uint16_t i = 0; i < ENTRY_COUNT; i++) {
            entries[i].clear();
        }
    }

    // Array access
    Entry& operator[](uint16_t index) {
        return entries[index];
    }

    const Entry& operator[](uint16_t index) const {
        return entries[index];
    }

    // Direct entry access
    Entry* get_entries() {
        return entries;
    }

    const Entry* get_entries() const {
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

    // Get next level table using recursive mapping - only available for P4, P3, P2
    // P1 doesn't have this method (compile error if you try to call it)
    // Two versions: non-const returns mutable pointer, const returns const pointer
    template<int L = Level>
    typename enable_if<(L > 1), Table<L - 1>*>::type
    get_next_table(size_t index) {
        if (!entries[index].is_present() || entries[index].is_huge()) {
            return nullptr;
        }
        return reinterpret_cast<Table<L - 1>*>(next_table_address(index));
    }

    template<int L = Level>
    typename enable_if<(L > 1), const Table<L - 1>*>::type
    get_next_table(size_t index) const {
        if (!entries[index].is_present() || entries[index].is_huge()) {
            return nullptr;
        }
        return reinterpret_cast<const Table<L - 1>*>(next_table_address(index));
    }

    // Get or create next level table - allocates a frame if entry is not present
    template<int L = Level, typename Allocator>
    typename enable_if<(L > 1), Table<L - 1>*>::type
    next_table_create(size_t index, Allocator& allocator) {
        if (entries[index].is_present()) {
            return get_next_table<L - 1>(index);
        }

        // Allocate a new frame for the table
        auto frame = allocator.allocate_frame();
        if (frame.number == 0) {
            return nullptr;  // Out of memory
        }

        // Set the entry to point to the new table with default flags
        entries[index].set(frame.start_address(), Entry::PRESENT | Entry::WRITABLE);

        // Get the newly created table and zero it
        auto table = reinterpret_cast<Table<L - 1>*>(next_table_address(index));
        table->clear();

        return table;
    }

    // Print table entries
    // recursive_level: 0 = current level only, 1 = one level down, -1 = all levels
    void print(VgaOutStream& stream, int recursive_level = 0, int indent = 0) const;

private:
    // Calculate next level table address using recursive page table mapping
    // Shift current address left by 9 bits (drops highest index, adds space for new index)
    // OR with index << 12 (adds the new index in the page offset position)
    VirtualAddress next_table_address(size_t index) const {
        VirtualAddress table_address = reinterpret_cast<uint64_t>(this);
        return (table_address << 9) | (index << 12);
    }

} __attribute__((aligned(4096)));

// Verify size for each instantiation
static_assert(sizeof(P4Table) == 4096, "P4Table must be 4096 bytes");
static_assert(sizeof(P3Table) == 4096, "P3Table must be 4096 bytes");
static_assert(sizeof(P2Table) == 4096, "P2Table must be 4096 bytes");
static_assert(sizeof(P1Table) == 4096, "P1Table must be 4096 bytes");

}

#endif //MAIN_PAGETABLE_H