//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#include "bootinfo.hpp"
#include "virtual/BumpAllocator.h"

// Forward declarations
class GDT;
class IDT;

namespace memory {

    extern Allocator* kernel_heap;
    extern FrameAllocator* frame_allocator;

    void init(BootInfo& boot_info, GDT& gdt, IDT& idt);
}

#endif //MAIN_MEMORY_H
