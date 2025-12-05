//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H
#include <stdint.h>

#include "c3.h"
#include "Table.h"
#include "Entry.h"
#include "memory/frame.h"
#include "memory/frame_allocator.h"


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
            ASSERT(address < 0x0000800000000000 || address >= 0xffff800000000000, "Invalid address");
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

    class ActivePageTable {
        P4Table *p4_table;
    public:
        static ActivePageTable& instance();

        rnt::Optional<PhysicalAddress> translate(VirtualAddress vaddr);
        void map_to(Page page, memory::Frame frame, uint64_t flags, memory::FrameAllocator& allocator);
        void map(Page page, uint64_t flags, memory::FrameAllocator& allocator);
        void identity_map(memory::Frame frame, uint64_t flags, memory::FrameAllocator& allocator);
        void unmap(Page page, memory::FrameAllocator& allocator);
        void print(VgaOutStream &stream, uint8_t recursive_level) const {
            p4_table->print(stream, recursive_level);
        }


    private:
        ActivePageTable(): p4_table(reinterpret_cast<P4Table *>(RECURSIVE_P4_ADDR)) {}
        rnt::Optional<memory::Frame> translate_page(Page page);
        static ActivePageTable instance_;
    };


}

#endif //MAIN_PAGE_H
