#include "bootinfo.hpp"
#include "vga.hpp"
#include "runtime/optional.h"

const Multiboot2Tag* Multiboot2Tag::next() const {
    uint32_t aligned_size = (size + 7) & ~7;  // Align to 8 bytes
    return reinterpret_cast<const Multiboot2Tag*>(
        reinterpret_cast<uintptr_t>(this) + aligned_size
    );
}

void BootInfo::print_size(VgaOutStream& stream, uint64_t bytes) {
    uint64_t kb = bytes / 1024;
    uint64_t mb = kb / 1024;
    uint64_t gb = mb / 1024;

    if (gb >= 1) {
        stream << gb << " GB";
    } else if (mb >= 1) {
        stream << mb << " MB";
    } else {
        stream << kb << " KB";
    }
}

const Multiboot2Tag* BootInfo::tags_begin() const {
    return reinterpret_cast<const Multiboot2Tag*>(
        reinterpret_cast<uintptr_t>(this) + 8
    );
}

const Multiboot2Tag* BootInfo::find_tag(uint32_t type) const {
    for (const Multiboot2Tag* tag = tags_begin();
         tag->type != Multiboot2Tag::END;
         tag = tag->next()) {
        if (tag->type == type) {
            return tag;
        }
    }
    return nullptr;
}

bool BootInfo::has_cmdline() const {
    return find_tag(Multiboot2Tag::CMDLINE) != nullptr;
}

bool BootInfo::has_boot_loader_name() const {
    return find_tag(Multiboot2Tag::BOOT_LOADER_NAME) != nullptr;
}

bool BootInfo::has_memory_info() const {
    return find_tag(Multiboot2Tag::BASIC_MEMINFO) != nullptr;
}

bool BootInfo::has_memory_map() const {
    return find_tag(Multiboot2Tag::MMAP) != nullptr;
}

bool BootInfo::has_elf_sections() const {
    return find_tag(Multiboot2Tag::ELF_SECTIONS) != nullptr;
}

const char* BootInfo::get_cmdline() const {
    auto tag = static_cast<const Multiboot2TagString*>(find_tag(Multiboot2Tag::CMDLINE));
    return tag ? tag->get_string() : nullptr;
}

const char* BootInfo::get_boot_loader_name() const {
    auto tag = static_cast<const Multiboot2TagString*>(find_tag(Multiboot2Tag::BOOT_LOADER_NAME));
    return tag ? tag->get_string() : nullptr;
}

uint32_t BootInfo::get_mem_lower() const {
    auto tag = static_cast<const Multiboot2TagBasicMeminfo*>(find_tag(Multiboot2Tag::BASIC_MEMINFO));
    return tag ? tag->mem_lower : 0;
}

uint32_t BootInfo::get_mem_upper() const {
    auto tag = static_cast<const Multiboot2TagBasicMeminfo*>(find_tag(Multiboot2Tag::BASIC_MEMINFO));
    return tag ? tag->mem_upper : 0;
}

uint64_t BootInfo::get_total_memory() const {
    return static_cast<uint64_t>(get_mem_lower()) * 1024 +
           static_cast<uint64_t>(get_mem_upper()) * 1024 +
           1024 * 1024;  // Add 1MB for the gap
}

const Multiboot2TagMmap* BootInfo::get_memory_map() const {
    return static_cast<const Multiboot2TagMmap*>(find_tag(Multiboot2Tag::MMAP));
}

rnt::Optional<const Multiboot2TagElfSections *> BootInfo::get_elf_sections() const {
    if (!has_elf_sections()) {
        return rnt::Optional<const Multiboot2TagElfSections *>();
    }
    return static_cast<const Multiboot2TagElfSections*>(find_tag(Multiboot2Tag::ELF_SECTIONS));
}

void BootInfo::print(VgaOutStream& stream) const {
    stream << "=== Boot Information ===" << VgaOutStream::endl;
    stream << "Total size: " << total_size << " bytes" << VgaOutStream::endl;
    stream << VgaOutStream::endl;

    // Boot loader name
    if (has_boot_loader_name()) {
        stream << "Bootloader: " << get_boot_loader_name() << VgaOutStream::endl;
    }

    // Command line
    if (has_cmdline()) {
        const char* cmdline = get_cmdline();
        stream << "Cmdline: " << (cmdline ? cmdline : "(empty)") << VgaOutStream::endl;
    }

    stream << VgaOutStream::endl;

    // Memory info
    if (has_memory_info()) {
        stream << "Memory Lower: " << get_mem_lower() << " KB" << VgaOutStream::endl;
        stream << "Memory Upper: " << get_mem_upper() << " KB" << VgaOutStream::endl;
        stream << "Total Memory: ";
        print_size(stream, get_total_memory());
        stream << VgaOutStream::endl;
    }

    stream << VgaOutStream::endl;

    // Memory map
    if (has_memory_map()) {
        stream << "Memory Map:" << VgaOutStream::endl;
        auto mmap = get_memory_map();
        for (auto entry = mmap->entries_begin(); entry != mmap->entries_end(); ++entry) {
            if (!entry->is_available()) continue;
            stream << "  0x";
            stream << entry->addr << " - 0x";
            stream << (entry->addr + entry->len) << " (";
            print_size(stream, entry->len);
            stream << ") ";

            if (entry->is_available()) {
                stream << "RAM";
            } else if (entry->is_reserved()) {
                stream << "Reserved";
            } else if (entry->is_acpi_reclaimable()) {
                stream << "ACPI Reclaimable";
            } else if (entry->is_acpi_nvs()) {
                stream << "ACPI NVS";
            } else if (entry->is_bad()) {
                stream << "Bad Memory";
            } else {
                stream << "Unknown";
            }

            stream << VgaOutStream::endl;
        }
    }

    // stream << VgaOutStream::endl;
    //
    // // ELF sections
    // if (has_elf_sections()) {
    //     stream << "Kernel Sections:" << VgaOutStream::endl;
    //     auto elf = get_elf_sections();
    //     for (auto section = elf->sections_begin(); section != elf->sections_end(); ++section) {
    //         // Skip sections with no size
    //         if (section->size == 0) continue;
    //
    //         const char* name = elf->get_section_name(section);
    //         stream << "  " << (name ? name : "(unnamed)");
    //         stream << " @ 0x" << section->addr;
    //         stream << " size=" << section->size;
    //         stream << " flags=";
    //         if (section->flags & 0x1) stream << "W";
    //         if (section->flags & 0x2) stream << "A";
    //         if (section->flags & 0x4) stream << "X";
    //         stream << VgaOutStream::endl;
    //     }
    // }
}
