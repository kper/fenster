//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#include <stdint.h>
#include "paging/paging.h"
#include "paging/Table.h"
#include "paging/Entry.h"
#include "paging/c3.h"

namespace memory {

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
bool map_to(paging::Page page, paging::Frame frame, uint64_t flags, Allocator& allocator) {
    using namespace paging;

    // Get P4 table
    P4Table* p4 = c3::get_virt_p4_table();

    // Walk down the page table hierarchy, creating tables as needed
    P3Table* p3 = p4->next_table_create(page.p4_index(), allocator);
    if (!p3) return false;  // Out of memory

    P2Table* p2 = p3->next_table_create(page.p3_index(), allocator);
    if (!p2) return false;  // Out of memory

    P1Table* p1 = p2->next_table_create(page.p2_index(), allocator);
    if (!p1) return false;  // Out of memory

    // Verify the entry is unused
    Entry& entry = (*p1)[page.p1_index()];
    if (!entry.is_unused()) {
        // Entry already in use - this is an error condition
        return false;
    }

    // Set the entry to map to the frame with PRESENT flag
    entry.set(frame.start_address(), flags | Entry::PRESENT);
    return true;
}

}

#endif //MAIN_MEMORY_H