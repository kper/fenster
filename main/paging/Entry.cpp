#include "Entry.h"
#include "../vga.hpp"

void paging::Entry::print(VgaOutStream& stream) const {
    if (!is_present()) {
        stream << "(not present)";
        return;
    }

    stream << "addr=" << hex << get_address() << " [";

    if (is_present()) stream << "P";
    if (is_writable()) stream << "W";
    if (is_user_accessible()) stream << "U";
    if (is_huge()) stream << "H";
    if (is_accessed()) stream << "A";
    if (is_dirty()) stream << "D";
    if (!is_executable()) stream << "NX";

    stream << "]";
}
