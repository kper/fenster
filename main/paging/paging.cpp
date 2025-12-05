//
// Created by Johannes Zottele on 04.12.25.
//

#include "paging.h"
#include "Table.h"
#include "Entry.h"
#include "c3.h"
#include "panic.h"
#include "vga.hpp"
#include "memory/frame.h"

namespace paging {

    void remap_the_kernel(memory::FrameAllocator &allocator, BootInfo &boot_info) {
        using vga::out;
        auto temp_raw_page = Page(0xcafebabe);
        auto temp_page = TemporaryPage(temp_raw_page, allocator);

        auto active_table = ActivePageTable::instance();
        active_table.print(vga::out, 1);
        temp_raw_page.debug_print(vga::out);
        auto new_table_frame = allocator.allocate_frame().expect("Out of memory");
        auto new_table = InactivePageTable(new_table_frame, active_table, temp_page);


        active_table.with(new_table, temp_page, [&boot_info, &allocator](ActivePageTable& map) {
            auto elf = boot_info.get_elf_sections().expect("Elf sections required");
            for (auto section = elf->sections_begin(); section != elf->sections_end(); ++section) {
                if (section->size == 0) {
                    continue;
                }
                out << "Processing " << elf->get_section_name(section) << "\n";
                ASSERT(section->addr % PAGE_SIZE == 0, "Elf sections must be page alined");
                out << "mapping section at addr: " << hex << section->addr << ", size: " << size << section->size << out.endl;
                auto flags = PageFlags{.writable = true};

                auto start_frame = memory::Frame::containing_address(section->addr);
                auto end_frame = memory::Frame::containing_address(section->addr + section->size - 1);
                for (uint64_t i = start_frame.number; i <= end_frame.number; ++i) {
                    auto frame = memory::Frame(i);
                    map.identity_map(frame, flags, allocator);
                }
            }

            // identity map the VGA text buffer
            auto vga_buffer_frame = memory::Frame::containing_address(0xb8000); // new
            map.identity_map(vga_buffer_frame, PageFlags {.writable = true}, allocator); // new
        });

        active_table.swap(new_table);
    }


    // PageFlags implementations
    uint64_t PageFlags::to_raw() const {
        uint64_t flags = Entry::PRESENT;  // Always present
        if (writable) flags |= Entry::WRITABLE;
        if (user_accessible) flags |= Entry::USER;
        if (write_through) flags |= Entry::WRITE_THROUGH;
        if (no_cache) flags |= Entry::NO_CACHE;
        if (no_execute) flags |= Entry::NO_EXECUTE;
        return flags;
    }

    PageFlags PageFlags::kernel_readonly() {
        return PageFlags{};
    }

    PageFlags PageFlags::kernel_readwrite() {
        PageFlags flags;
        flags.writable = true;
        return flags;
    }

    PageFlags PageFlags::user_readonly() {
        PageFlags flags;
        flags.user_accessible = true;
        return flags;
    }

    PageFlags PageFlags::user_readwrite() {
        PageFlags flags;
        flags.writable = true;
        flags.user_accessible = true;
        return flags;
    }

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

    void ActivePageTable::map_to(Page page, memory::Frame frame, PageFlags flags, memory::FrameAllocator &allocator) {
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

        // Set the entry to map to the frame with converted flags
        entry.set(frame.start_address(), flags.to_raw());
    }

    void ActivePageTable::map(Page page, PageFlags flags, memory::FrameAllocator &allocator) {
        auto frame = allocator.allocate_frame();
        ASSERT(frame.has_value(), "out of memory");
        map_to(page, frame.value(), flags, allocator);
    }

    void ActivePageTable::identity_map(memory::Frame frame, PageFlags flags, memory::FrameAllocator &allocator) {
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

    void ActivePageTable::swap(InactivePageTable& inactive_page_table) {
        auto p4_frame = c3::get_frame();
        auto new_p4_frame = inactive_page_table.p4_frame;
        inactive_page_table.p4_frame = p4_frame;
        c3::set_phys_addr(new_p4_frame.start_address());
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

    template<typename F>
    void ActivePageTable::with(InactivePageTable& table, TemporaryPage& temporary_page, F&& f) {
        auto backup = memory::Frame::containing_address(c3::get_phys_addr());
        // map the temporary page on the current physical address of the active page table
        auto temp_p4 = temporary_page.map_table_frame(backup, *this);

        // point the recursive index to the inactive page table
        p4_table->get_entries()[RECURSIVE_INDEX].set(table.p4_frame.start_address(), Entry::PRESENT | Entry::WRITABLE);
        c3::flush();

        // execute lambda in this context
        f(*this);

        // restore the recursive index to the old state
        temp_p4->get_entries()[RECURSIVE_INDEX].set(backup.start_address(), Entry::PRESENT | Entry::WRITABLE);
    }


    /**
     * The tiny allocator has control over exaclty 3 frames that can be used
     * by the TemporaryPage.
     */
    TinyAllocator::TinyAllocator(FrameAllocator& allocator) {
        for (int i = 0; i < 3; i++) {
            frames[i] = allocator.allocate_frame().expect("Out of memory");
            available[i] = true;
        }
    }

    rnt::Optional<memory::Frame> TinyAllocator::allocate_frame() {
        for (int i = 0; i < 3; i++) {
            if (available[i]) {
                available[i] = false;
                return frames[i];
            }
        }
        return rnt::Optional<memory::Frame>();
    }

    void TinyAllocator::deallocate_frame(memory::Frame frame) {
        // we take back only the frames we own
        for (int i = 0; i < 3; i++) {
            if (frames[i].number == frame.number) {
                ASSERT(!available[i], "Frame was already deallocated");
                available[i] = true;
                return;
            }
        }
    }


    TemporaryPage::TemporaryPage(Page page, memory::FrameAllocator& allocator): page(page), allocator(TinyAllocator(allocator)) {
    }

    VirtualAddress TemporaryPage::map(memory::Frame frame, ActivePageTable &active_table) {
        ASSERT(active_table.translate_page(page).is_empty(), "Temporary page is already mapped");
        active_table.map_to(page, frame, PageFlags{.writable = true}, allocator);
        return page.start_addr();
    }

    void TemporaryPage::unmap(ActivePageTable &active_table) {
        active_table.unmap(page, allocator);
    }

    P1Table* TemporaryPage::map_table_frame(memory::Frame frame, ActivePageTable &active_page_table) {
        return reinterpret_cast<P1Table*>(map(frame, active_page_table));
    }



}
