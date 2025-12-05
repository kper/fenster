//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H
#include <stdint.h>

#include "Table.h"
#include "Entry.h"
#include "c3.h"
#include "memory/frame.h"

using PhysicalAddress = uint64_t;
using VirtualAddress = uint64_t;

namespace paging {
    // Import PAGE_SIZE from memory namespace
    using memory::PAGE_SIZE;

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
     * Translate a virtual page to its physical frame
     * @param page Virtual page to translate
     * @return Physical frame, or Frame(0) if not mapped
     */
    memory::Frame page_to_frame(Page page);

    /**
     * Translate a virtual address to its physical address
     * @param virtual_address Virtual address to translate
     * @return Physical address, or 0 if not mapped
     */
    PhysicalAddress virt_to_phys_addr(VirtualAddress virtual_address);

}

#endif //MAIN_PAGE_H
