//
// Created by Johannes Zottele on 05.12.25.
//

#ifndef MAIN_REGS_H
#define MAIN_REGS_H
#include "paging/paging.h"

namespace cr2 {
    // Returns the Page Fault Linear Address
    VirtualAddress get_pfla() {
        uint64_t cr3_value;
        asm volatile("mov %%cr3, %0" : "=r"(cr3_value));
        return cr3_value;
    }
}

#endif //MAIN_REGS_H