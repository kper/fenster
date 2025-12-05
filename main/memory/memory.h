//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#include <stdint.h>
#include "paging/Table.h"
#include "paging/Entry.h"
#include "paging/c3.h"
#include "panic.h"

using PhysicalAddress = uint64_t;

namespace paging {
    constexpr uint64_t PAGE_SIZE = 4096;
    struct Page;
}

namespace memory {

/**
 * Represents a physical memory frame (4KB page)
 */
struct Frame {
    uint64_t number;

    Frame() : number(0) {}
    explicit Frame(uint64_t num) : number(num) {}

    uint64_t start_address() const {
        return paging::PAGE_SIZE * number;
    }

    static Frame containing_address(PhysicalAddress addr) {
        return Frame(addr / paging::PAGE_SIZE);
    }

    Frame clone() const {
        return Frame(number);
    }

    bool operator==(const Frame& other) const {
        return number == other.number;
    }

    bool operator!=(const Frame& other) const {
        return number != other.number;
    }
};

/**
 * Map a virtual page to a physical frame with specified flags
 * Creates intermediate page tables if needed using the provided allocator
 *
 * @param page Virtual page to map
 * @param frame Physical frame to map to
 * @param flags Page entry flags (e.g., PRESENT | WRITABLE)
 * @param allocator Frame allocator for creating intermediate tables
 */
template<typename Allocator>
void map_to(paging::Page page, paging::Frame frame, uint64_t flags, Allocator& allocator) {
    using namespace paging;

    // Get P4 table
    P4Table* p4 = c3::get_virt_p4_table();

    // Walk down the page table hierarchy, creating tables as needed
    P3Table* p3 = p4->next_table_create(page.p4_index(), allocator);
    ASSERT(p3 != nullptr, "Out of memory allocating P3 table");

    P2Table* p2 = p3->next_table_create(page.p3_index(), allocator);
    ASSERT(p2 != nullptr, "Out of memory allocating P2 table");

    P1Table* p1 = p2->next_table_create(page.p2_index(), allocator);
    ASSERT(p1 != nullptr, "Out of memory allocating P1 table");

    // Verify the entry is unused
    Entry& entry = (*p1)[page.p1_index()];
    ASSERT(entry.is_unused(), "Page already mapped");

    // Set the entry to map to the frame with PRESENT flag
    entry.set(frame.start_address(), flags | Entry::PRESENT);
}

}

#endif //MAIN_MEMORY_H