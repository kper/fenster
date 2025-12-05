//
// Created by Johannes Zottele on 04.12.25.
//

#include "paging.h"
#include "Table.h"
#include "Entry.h"
#include "c3.h"
#include "panic.h"
#include "memory/frame.h"

namespace paging {

    // the actual instance in static memory
    ActivePageTable ActivePageTable::instance_;

    ActivePageTable& ActivePageTable::instance() {
        return instance_;
    }

    rnt::Optional<PhysicalAddress> ActivePageTable::translate(VirtualAddress vaddr) {
        auto offset = vaddr % PAGE_SIZE;
        auto page = Page::containing_address(vaddr);
        auto frame = translate_page(page);
        return frame.is_empty()
        ? rnt::Optional<PhysicalAddress>()
        : frame.value().number * PAGE_SIZE + offset;
    }

    void ActivePageTable::map_to(Page page, memory::Frame frame, uint64_t flags, memory::FrameAllocator &allocator) {
        // Walk down the page table hierarchy, creating tables as needed
        auto* p3 = p4_table->next_table_create(page.p4_index(), allocator);
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

    void ActivePageTable::map(Page page, uint64_t flags, memory::FrameAllocator &allocator) {
        auto frame = allocator.allocate_frame();
        ASSERT(frame.has_value(), "out of memory");
        map_to(page, frame.value(), flags, allocator);
    }

    void ActivePageTable::identity_map(memory::Frame frame, uint64_t flags, memory::FrameAllocator &allocator) {
        auto page = Page::containing_address(frame.start_address());
        map_to(page, frame, flags, allocator);
    }

    void ActivePageTable::unmap(Page page, memory::FrameAllocator &allocator) {
        ASSERT(translate(page.start_addr()).has_value(), "Page not mapped");

        auto* p3 = p4_table->get_next_table(page.p4_index());
        ASSERT(p3 != nullptr, "Huge pages not supported by mapping code");
        P2Table* p2 = p3->get_next_table(page.p3_index());
        ASSERT(p2 != nullptr, "Huge pages not supported by mapping code");
        P1Table* p1 = p2->get_next_table(page.p2_index());

        auto &entry = p1->get_entries()[page.p1_index()];
        auto frame = entry.get_frame().value();
        entry.clear();
        allocator.deallocate_frame(frame);
        // flush the lookaside buffer (TLB)
        c3::flush();
    }


    rnt::Optional<memory::Frame> ActivePageTable::translate_page(Page page) {
        // Walk through the page table hierarchy
        P4Table* p4 = c3::get_virt_p4_table();

        // Get P3 table
        P3Table* p3 = p4->get_next_table(page.p4_index());
        if (!p3) {
            return rnt::Optional<memory::Frame>();
        }

        // Get P2 table
        P2Table* p2 = p3->get_next_table(page.p3_index());
        if (!p2) {
            return rnt::Optional<memory::Frame>();
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
            return rnt::Optional<memory::Frame>();
        }

        // Get the final entry
        const Entry& p1_entry = (*p1)[page.p1_index()];
        return p1_entry.get_frame();
    }
}
