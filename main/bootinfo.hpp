#ifndef MAIN_BOOTINFO_H
#define MAIN_BOOTINFO_H
#include <stdint.h>

#include "runtime/optional.h"

class VgaOutStream;

/**
 * Multiboot2 boot information (tag-based format)
 * https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
 */

// Base tag structure
struct Multiboot2Tag {
    uint32_t type;
    uint32_t size;

    enum Type : uint32_t {
        END = 0,
        CMDLINE = 1,
        BOOT_LOADER_NAME = 2,
        MODULE = 3,
        BASIC_MEMINFO = 4,
        BOOTDEV = 5,
        MMAP = 6,
        VBE = 7,
        FRAMEBUFFER = 8,
        ELF_SECTIONS = 9,
        APM = 10,
        EFI32 = 11,
        EFI64 = 12,
        SMBIOS = 13,
        ACPI_OLD = 14,
        ACPI_NEW = 15,
        NETWORK = 16,
        EFI_MMAP = 17,
        EFI_BS = 18,
        EFI32_IH = 19,
        EFI64_IH = 20,
        LOAD_BASE_ADDR = 21
    };

    const Multiboot2Tag* next() const;
} __attribute__((packed));

struct Multiboot2TagString : public Multiboot2Tag {
    char string[0];  // Flexible array member

    const char* get_string() const { return string; }
} __attribute__((packed));

struct Multiboot2TagBasicMeminfo : public Multiboot2Tag {
    uint32_t mem_lower;
    uint32_t mem_upper;
} __attribute__((packed));

struct MemoryArea {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;

    enum Type : uint32_t {
        AVAILABLE = 1,
        RESERVED = 2,
        ACPI_RECLAIMABLE = 3,
        ACPI_NVS = 4,
        BAD_MEMORY = 5
    };

    bool is_available() const { return type == AVAILABLE; }
    bool is_reserved() const { return type == RESERVED; }
    bool is_acpi_reclaimable() const { return type == ACPI_RECLAIMABLE; }
    bool is_acpi_nvs() const { return type == ACPI_NVS; }
    bool is_bad() const { return type == BAD_MEMORY; }
} __attribute__((packed));

struct Multiboot2TagMmap : public Multiboot2Tag {
    uint32_t entry_size;
    uint32_t entry_version;
    MemoryArea entries[0];  // Flexible array member

    const MemoryArea* entries_begin() const {
        return entries;
    }

    const MemoryArea* entries_end() const {
        uint32_t entries_size = size - sizeof(Multiboot2Tag) - sizeof(entry_size) - sizeof(entry_version);
        uint32_t num_entries = entries_size / entry_size;
        return &entries[num_entries];
    }
} __attribute__((packed));

struct Multiboot2ElfSection {
    uint32_t name;       // Section name (string table index)
    uint32_t type;       // Section type
    uint64_t flags;      // Section flags
    uint64_t addr;       // Section virtual address
    uint64_t offset;     // Section file offset
    uint64_t size;       // Section size in bytes
    uint32_t link;       // Link to another section
    uint32_t info;       // Additional section information
    uint64_t addralign;  // Section alignment
    uint64_t entsize;    // Entry size if section holds table

    // ELF section flags
    static constexpr uint64_t SHF_ALLOC = 0x2;  // Section occupies memory during execution

    bool is_allocated() const {
        return flags & SHF_ALLOC;
    }
} __attribute__((packed));

struct Multiboot2TagElfSections : public Multiboot2Tag {
    uint32_t num;           // Number of sections
    uint32_t entsize;       // Size of each entry
    uint32_t shndx;         // String table section index
    Multiboot2ElfSection sections[0];  // Flexible array member

    const Multiboot2ElfSection* sections_begin() const {
        return sections;
    }

    const Multiboot2ElfSection* sections_end() const {
        return &sections[num];
    }

    const char* get_section_name(const Multiboot2ElfSection* section) const {
        if (shndx >= num) return nullptr;
        const Multiboot2ElfSection* strtab = &sections[shndx];
        if (section->name >= strtab->size) return nullptr;
        return reinterpret_cast<const char*>(strtab->addr + section->name);
    }
} __attribute__((packed));

struct Multiboot2TagFramebuffer : public Multiboot2Tag {
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint8_t  reserved;

    enum Type : uint8_t {
        INDEXED = 0,
        RGB = 1,
        EGA_TEXT = 2
    };

    // For RGB mode (type == 1)
    struct {
        uint8_t red_field_position;
        uint8_t red_mask_size;
        uint8_t green_field_position;
        uint8_t green_mask_size;
        uint8_t blue_field_position;
        uint8_t blue_mask_size;
    } color_info;

    bool is_rgb() const { return framebuffer_type == RGB; }
    uint32_t* get_buffer() const { return reinterpret_cast<uint32_t*>(framebuffer_addr); }
} __attribute__((packed));

/**
 * Multiboot2 information structure
 * EBX points to this structure
 */
struct BootInfo {
private:
    uint32_t total_size;
    uint32_t reserved __attribute__((unused));

    static void print_size(VgaOutStream& stream, uint64_t bytes);

public:
    uint32_t get_total_size() const { return total_size; }

    // Get first tag
    const Multiboot2Tag* tags_begin() const;
    const Multiboot2Tag* find_tag(uint32_t type) const;

    // Convenience methods
    bool has_cmdline() const;
    bool has_boot_loader_name() const;
    bool has_memory_info() const;
    bool has_memory_map() const;
    bool has_elf_sections() const;
    bool has_framebuffer() const;

    const char* get_cmdline() const;
    const char* get_boot_loader_name() const;
    uint32_t get_mem_lower() const;
    uint32_t get_mem_upper() const;
    uint64_t get_total_memory() const;
    const Multiboot2TagMmap* get_memory_map() const;

    rnt::Optional<const Multiboot2TagElfSections *> get_elf_sections() const;
    rnt::Optional<const Multiboot2TagFramebuffer *> get_framebuffer() const;

    void print(VgaOutStream& stream) const;

} __attribute__((packed));



#endif //MAIN_BOOTINFO_H