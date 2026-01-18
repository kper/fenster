//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#include "bootinfo.hpp"
#include "virtual/BumpAllocator.h"

namespace memory {

    extern Allocator* kernel_heap;
    extern FrameAllocator* frame_allocator;

    // Initialize memory, remap kernel to high addresses, and jump to high half
    // This function does NOT return! It jumps to kernel_main_high()
    void init_and_jump_high(BootInfo& boot_info) __attribute__((noreturn));
    void init_heap();
}

#endif //MAIN_MEMORY_H
