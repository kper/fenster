//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_PAGEENTRY_H
#define MAIN_PAGEENTRY_H

#include <stdint.h>

#include "Page.h"

class VgaOutStream;

namespace paging {


/**
 * x86-64 Page Table Entry
 * Represents a single entry in any level of the page table hierarchy (P4/P3/P2/P1)
 */
class Entry {
private:
    uint64_t entry;

public:
    // Flag bit positions
    enum Flags : uint64_t {
        PRESENT        = 1 << 0,   // Page is present in memory
        WRITABLE       = 1 << 1,   // Page is writable
        USER           = 1 << 2,   // User mode accessible
        WRITE_THROUGH  = 1 << 3,   // Write-through caching
        NO_CACHE       = 1 << 4,   // Disable cache
        ACCESSED       = 1 << 5,   // Page was accessed
        DIRTY          = 1 << 6,   // Page was written to (only for P1)
        HUGE           = 1 << 7,   // Huge page (2MB in P2, 1GB in P3)
        GLOBAL         = 1 << 8,   // Global page (not flushed on TLB invalidation)
        NO_EXECUTE     = 1ULL << 63 // Disable execution
    };

    Entry() : entry(0) {}
    explicit Entry(uint64_t value) : entry(value) {}

    // Flag checking
    bool is_present() const { return entry & PRESENT; }
    bool is_writable() const { return entry & WRITABLE; }
    bool is_user_accessible() const { return entry & USER; }
    bool is_huge() const { return entry & HUGE; }
    bool is_accessed() const { return entry & ACCESSED; }
    bool is_dirty() const { return entry & DIRTY; }
    bool is_executable() const { return !(entry & NO_EXECUTE); }

    // Flag setting
    void set_present(bool value) { set_flag(PRESENT, value); }
    void set_writable(bool value) { set_flag(WRITABLE, value); }
    void set_user_accessible(bool value) { set_flag(USER, value); }
    void set_huge(bool value) { set_flag(HUGE, value); }
    void set_no_execute(bool value) { set_flag(NO_EXECUTE, value); }

    // Physical address (bits 12-51)
    PhysicalAddress get_address() const {
        return entry & 0x000FFFFFFFFFF000ULL;
    }

    void set_address(PhysicalAddress addr) {
        entry = (entry & 0xFFF0000000000FFFULL) | (addr & 0x000FFFFFFFFFF000ULL);
    }

    // Set address and flags in one operation
    void set(PhysicalAddress addr, uint64_t flags) {
        entry = (addr & 0x000FFFFFFFFFF000ULL) | (flags & 0xFFF0000000000FFFULL);
    }

    // Get raw value
    uint64_t get_raw() const { return entry; }
    void set_raw(uint64_t value) { entry = value; }

    // Clear entry
    void clear() { entry = 0; }

    // Print entry
    void print(VgaOutStream& stream) const;

private:
    void set_flag(uint64_t flag, bool value) {
        if (value) {
            entry |= flag;
        } else {
            entry &= ~flag;
        }
    }
} __attribute__((packed));

static_assert(sizeof(Entry) == 8, "PageTableEntry must be 8 bytes");

}

#endif //MAIN_PAGEENTRY_H

