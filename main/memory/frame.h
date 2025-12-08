//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_FRAME_H_INCLUDE
#define MAIN_FRAME_H_INCLUDE

#include <stdint.h>
#include "constants.h"

using PhysicalAddress = uint64_t;

namespace memory {

/**
 * Represents a physical memory frame (4KB page)
 */
struct Frame {
    uint64_t number;

    Frame() : number(0) {}
    explicit Frame(uint64_t num) : number(num) {}

    uint64_t start_address() const {
        return PAGE_SIZE * number;
    }

    static Frame containing_address(PhysicalAddress addr) {
        return Frame(addr / PAGE_SIZE);
    }

    Frame clone() const {
        return Frame(number);
    }

    bool operator==(const Frame& other) const {
        return number == other.number;
    }

    bool operator!=(const Frame& other) const {
        return number != other.number;
    }
};

}

#endif //MAIN_FRAME_H_INCLUDE
