#include "PageTable.h"
#include "../vga.hpp"

// Helper to print indentation
static void print_indent(VgaOutStream& stream, int indent) {
    for (int i = 0; i < indent; i++) {
        stream << "  ";
    }
}

// Level name helper
static const char* level_name(int level) {
    switch (level) {
        case 4: return "P4";
        case 3: return "P3";
        case 2: return "P2";
        case 1: return "P1";
        default: return "P?";
    }
}

// Helper to recursively print next level (SFINAE to prevent P1 from recursing)
template<int Level>
typename enable_if<(Level > 1), void>::type
print_next_level(const PageTableEntry& entry, VgaOutStream& stream, int recursive_level, int indent) {
    if (recursive_level != 0 && !entry.is_huge()) {
        auto* next = entry.get_next_table<PageTable<Level - 1>>();
        if (next) {
            int next_level = (recursive_level < 0) ? -1 : recursive_level - 1;
            next->print(stream, next_level, indent + 1);
        }
    }
}

template<int Level>
typename enable_if<(Level <= 1), void>::type
print_next_level(const PageTableEntry&, VgaOutStream&, int, int) {
    // P1 has no next level - do nothing
}

// Generic print implementation for all levels
template<int Level>
void PageTable<Level>::print(VgaOutStream& stream, int recursive_level, int indent) const {
    for (size_t i = 0; i < ENTRY_COUNT; i++) {
        const PageTableEntry& entry = entries[i];
        if (!entry.is_present()) continue;

        print_indent(stream, indent);
        stream << level_name(Level) << "[" << (uint32_t)i << "]: ";
        entry.print(stream);
        stream << VgaOutStream::endl;

        print_next_level<Level>(entry, stream, recursive_level, indent);
    }
}

// Explicit instantiations
template void PageTable<4>::print(VgaOutStream&, int, int) const;
template void PageTable<3>::print(VgaOutStream&, int, int) const;
template void PageTable<2>::print(VgaOutStream&, int, int) const;
template void PageTable<1>::print(VgaOutStream&, int, int) const;
