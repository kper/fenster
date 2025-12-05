//
// Created by Johannes Zottele on 04.12.25.
//

#include "paging.h"
#include "Table.h"
#include "Entry.h"
#include "c3.h"
#include "memory/frame.h"

namespace paging {

memory::Frame page_to_frame(Page page) {
    // Walk through the page table hierarchy
    P4Table* p4 = c3::get_virt_p4_table();

    // Get P3 table
    P3Table* p3 = p4->get_next_table(page.p4_index());
    if (!p3) {
        return memory::Frame(0);  // Page not mapped
    }

    // Get P2 table
    P2Table* p2 = p3->get_next_table(page.p3_index());
    if (!p2) {
        return memory::Frame(0);  // Page not mapped
    }

    // Check if this is a huge page (2MB)
    const Entry& p2_entry = (*p2)[page.p2_index()];
    if (p2_entry.is_huge()) {
        // For huge pages, the P2 entry contains the frame address
        // Frame number = (physical address / PAGE_SIZE) + P1 index offset
        uint64_t huge_frame_base = p2_entry.get_address() / PAGE_SIZE;
        return memory::Frame(huge_frame_base + page.p1_index());
    }

    // Get P1 table
    P1Table* p1 = p2->get_next_table(page.p2_index());
    if (!p1) {
        return memory::Frame(0);  // Page not mapped
    }

    // Get the final entry
    const Entry& p1_entry = (*p1)[page.p1_index()];
    if (!p1_entry.is_present()) {
        return memory::Frame(0);  // Page not mapped
    }

    // Return frame from physical address
    return memory::Frame::containing_address(p1_entry.get_address());
}

PhysicalAddress virt_to_phys_addr(VirtualAddress virtual_address) {
    auto offset = virtual_address % PAGE_SIZE;
    auto page = Page::containing_address(virtual_address);
    auto frame_nr = page_to_frame(page).number;
    return frame_nr == 0 ? 0 : frame_nr * PAGE_SIZE + offset;
}

}
