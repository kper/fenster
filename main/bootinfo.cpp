#include "bootinfo.hpp"

const Multiboot2Tag* Multiboot2Tag::next() const {
    uint32_t aligned_size = (size + 7) & ~7;  // Align to 8 bytes
    return reinterpret_cast<const Multiboot2Tag*>(
        reinterpret_cast<uintptr_t>(this) + aligned_size
    );
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
