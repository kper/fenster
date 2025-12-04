//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H
#include <stdint.h>

using PhysicalAddress = uint64_t;
using VirtualAddress = uint64_t;

namespace paging {

struct Page {
    uint64_t number;
};

}

#endif //MAIN_PAGE_H
