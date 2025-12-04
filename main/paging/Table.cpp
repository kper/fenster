#include "Table.h"
#include "../vga.hpp"

using namespace paging;

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

// Generic print implementation for all levels
template<int Level>
void Table<Level>::print(VgaOutStream& stream, int recursive_level, int indent) const {
    for (size_t i = 0; i < ENTRY_COUNT; i++) {
        const Entry& entry = entries[i];
        if (!entry.is_present()) continue;

        print_indent(stream, indent);
        stream << level_name(Level) << "[" << (uint32_t)i << "]: ";
        entry.print(stream);
        stream << VgaOutStream::endl;

        // Recursively print next level if requested (only for P4, P3, P2)
        if constexpr (Level > 1) {
            if (recursive_level != 0 && !entry.is_huge()) {
                auto* next = get_next_table(i);
                if (next) {
                    int next_level = (recursive_level < 0) ? -1 : recursive_level - 1;
                    next->print(stream, next_level, indent + 1);
                }
            }
        }
    }
}

// Explicit instantiations
template void Table<4>::print(VgaOutStream&, int, int) const;
template void Table<3>::print(VgaOutStream&, int, int) const;
template void Table<2>::print(VgaOutStream&, int, int) const;
template void Table<1>::print(VgaOutStream&, int, int) const;
