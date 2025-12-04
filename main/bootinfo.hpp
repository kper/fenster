#ifndef MAIN_BOOTINFO_H
#define MAIN_BOOTINFO_H

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

struct Multiboot2MmapEntry {
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
    Multiboot2MmapEntry entries[0];  // Flexible array member

    const Multiboot2MmapEntry* entries_begin() const {
        return entries;
    }

    const Multiboot2MmapEntry* entries_end() const {
        uint32_t entries_size = size - sizeof(Multiboot2Tag) - sizeof(entry_size) - sizeof(entry_version);
        uint32_t num_entries = entries_size / entry_size;
        return &entries[num_entries];
    }
} __attribute__((packed));

/**
 * Multiboot2 information structure
 * EBX points to this structure
 */
struct BootInfo {
private:
    uint32_t total_size;
    uint32_t reserved;

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

    const char* get_cmdline() const;
    const char* get_boot_loader_name() const;
    uint32_t get_mem_lower() const;
    uint32_t get_mem_upper() const;
    uint64_t get_total_memory() const;
    const Multiboot2TagMmap* get_memory_map() const;

} __attribute__((packed));



#endif //MAIN_BOOTINFO_H