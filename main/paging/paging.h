//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H
#include <stdint.h>

#include "Entry.h"
#include "Table.h"
#include "vga.hpp"
#include "memory/frame.h"
#include "memory/frame_allocator.h"


using PhysicalAddress = uint64_t;
using VirtualAddress = uint64_t;


namespace paging {
    // forward declarations
    class InactivePageTable;
    class TemporaryPage;

    // Import PAGE_SIZE from memory namespace
    using memory::PAGE_SIZE;

    // Higher-half kernel mapping constants
    // P4[510] maps to 0xFFFF800000000000 - 0xFFFF807FFFFFFFFF (512 GB)
    constexpr uint64_t KERNEL_OFFSET = 0xFFFF800000000000ULL;
    constexpr uint16_t KERNEL_P4_INDEX = 510;

    void remap_the_kernel(memory::FrameAllocator& allocator, BootInfo& boot_info);

    /**
     * Jump to higher-half kernel and execute continuation function
     * This function does not return! It adjusts RSP to high address and calls the continuation.
     * @param continuation Function to call after jumping to high addresses (must be __attribute__((noreturn)))
     */
    void jump_to_higher_half(void (*continuation)()) __attribute__((noreturn));

    void unmap_lower_half(memory::FrameAllocator& allocator);

    /**
     * Page flags for mapping operations
     * Provides a type-safe way to specify page table entry flags
     */
    struct PageFlags {
        bool writable = false;
        bool user_accessible = false;
        bool write_through = false;
        bool no_cache = false;
        bool no_execute = false;

        // Convert to raw flags for Entry
        uint64_t to_raw() const;

        // Static factory methods for common configurations
        static PageFlags kernel_readonly();
        static PageFlags kernel_readwrite();
        static PageFlags user_readonly();
        static PageFlags user_readwrite();
        static PageFlags from_elf_section(const Multiboot2ElfSection& elf_section);
    };

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

        void debug_print(VgaOutStream &out) const {
            out << "Page#" << hex << number << " addr: " << hex << start_addr()
            <<" P4[" << p4_index() << "] P3[" << p3_index()
            << "] P2[" << p2_index() << "] P1[" << p1_index() << "]" << out.endl;
        }
    };

    class ActivePageTable {
        P4Table *p4_table;
    public:
        static ActivePageTable& instance();

        rnt::Optional<PhysicalAddress> translate(VirtualAddress vaddr);

        // Map a page to a specific frame with given flags
        void map_to(Page page, memory::Frame frame, PageFlags flags, memory::FrameAllocator& allocator);

        // Map a page (allocates a frame automatically)
        void map(Page page, PageFlags flags, memory::FrameAllocator& allocator);

        // Identity map a frame (virtual address = physical address)
        void identity_map(memory::Frame frame, PageFlags flags, memory::FrameAllocator& allocator);

        // Unmap a page
        void unmap(Page page, memory::FrameAllocator& allocator);

        // Swaps the internal P4 table pointer between the active page and the
        // given inactive one. After this operation,
        // the active table will point to the previously inactive page table
        // and the inactive one to the previously active one
        void swap(InactivePageTable& inactive_page_table);

        template<typename F>
        void with(InactivePageTable& table, TemporaryPage& temporary_page, F&& f);

        void print(VgaOutStream &stream, uint8_t recursive_level) const {
            p4_table->print(stream, recursive_level);
        }

    private:
        ActivePageTable(): p4_table(reinterpret_cast<P4Table *>(RECURSIVE_P4_ADDR)) {}
        rnt::Optional<memory::Frame> translate_page(Page page);
        static ActivePageTable instance_;
        friend class TemporaryPage;
    };

    /**
     * The tiny allocator has control over exactly 3 frames that can be used
     * by the TemporaryPage.
     */
    class TinyAllocator: public memory::FrameAllocator {
        memory::Frame frames[3];
        bool          available[3];

    public:
        TinyAllocator(FrameAllocator& allocator);
        rnt::Optional<memory::Frame> allocate_frame();
        void deallocate_frame(memory::Frame frame);
    };

    class TemporaryPage {
        Page page;
        TinyAllocator allocator;

        public:
        TemporaryPage(Page page, memory::FrameAllocator& allocator);
        VirtualAddress map(memory::Frame frame, ActivePageTable& active_table);
        void unmap(ActivePageTable& active_table);
        P1Table* map_table_frame(memory::Frame frame, ActivePageTable& active_page_table);
    };

    class InactivePageTable {
        // frame which contains the inactive table
        friend ActivePageTable;

        public:
        memory::Frame p4_frame;
        explicit InactivePageTable(memory::Frame frame, ActivePageTable& active_table, TemporaryPage& temporary_page): p4_frame(frame) {
            // Create a p4 table frame on the temporary page (temporary page is only used to access the frame)
            auto table = temporary_page.map_table_frame(frame, active_table);
            // Reset garbage value at that frame
            table->clear();
            // Make the p4 table recursive
            table->get_entries()[RECURSIVE_INDEX].set(frame.start_address(), Entry::PRESENT | Entry::WRITABLE);
            // Unmap the temporary page, so it is free again. The p4_frame is still allocated by the original allocator.
            temporary_page.unmap(active_table);
        }
    };

}

#endif //MAIN_PAGE_H
