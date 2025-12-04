//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H
#include <stdint.h>

#include "Table.h"
#include "Entry.h"
#include "c3.h"

using PhysicalAddress = uint64_t;
using VirtualAddress = uint64_t;

namespace paging {

    constexpr uint64_t PAGE_SIZE = 4096;

    struct Page {
        uint64_t number;

        Page() : number(0) {}
        explicit Page(uint64_t number) : number(number) {}

        static Page containing_address(VirtualAddress address) {
            return Page(address / PAGE_SIZE);
        }

        VirtualAddress start_addr() const {
            return number * PAGE_SIZE;
        }

        uint16_t p4_index() const {
            return (number >> 27) & 0x1ff;
        }

        uint16_t p3_index() const {
            return ((number >> 18) & 0x1ff);
        }

        uint16_t p2_index() const {
            return (number >> 9) & 0x1ff;
        }

        uint16_t p1_index() const {
            return (number & 0x1ff);
        }
    };

    /**
     * Represents a physical memory frame (4KB page)
     */
    struct Frame {
        uint64_t number;

        Frame() : number(0) {}
        explicit Frame(uint64_t num) : number(num) {}

        uint64_t start_address() const {
            return PAGE_SIZE * number;
        }

        static Frame containing_address(PhysicalAddress addr) {
            return Frame(addr / PAGE_SIZE);
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


    inline Frame page_to_frame(Page page) {
        // Walk through the page table hierarchy
        P4Table* p4 = c3::get_virt_p4_table();

        // Get P3 table
        P3Table* p3 = p4->get_next_table(page.p4_index());
        if (!p3) {
            return Frame(0);  // Page not mapped
        }

        // Get P2 table
        P2Table* p2 = p3->get_next_table(page.p3_index());
        if (!p2) {
            return Frame(0);  // Page not mapped
        }

        // Check if this is a huge page (2MB)
        const Entry& p2_entry = (*p2)[page.p2_index()];
        if (p2_entry.is_huge()) {
            // For huge pages, the P2 entry contains the frame address
            // Frame number = (physical address / PAGE_SIZE) + P1 index offset
            uint64_t huge_frame_base = p2_entry.get_address() / PAGE_SIZE;
            return Frame(huge_frame_base + page.p1_index());
        }

        // Get P1 table
        P1Table* p1 = p2->get_next_table(page.p2_index());
        if (!p1) {
            return Frame(0);  // Page not mapped
        }

        // Get the final entry
        const Entry& p1_entry = (*p1)[page.p1_index()];
        if (!p1_entry.is_present()) {
            return Frame(0);  // Page not mapped
        }

        // Return frame from physical address
        return Frame::containing_address(p1_entry.get_address());
    }

    inline PhysicalAddress virt_to_phys_addr(VirtualAddress virtual_address) {
        auto offset = virtual_address % PAGE_SIZE;
        auto page = Page::containing_address(virtual_address);
        auto frame_nr = page_to_frame(page).number;
        return frame_nr == 0 ? 0 : frame_nr * PAGE_SIZE + offset;
    }

}

#endif //MAIN_PAGE_H
