//
// Created by flo on 19.01.26.
//

#ifndef MAIN_PROCESS_H
#define MAIN_PROCESS_H

#include "memory/memory.h"
#include "memory/virtual/BlockAllocator.h"


class Process {
public:
    memory::BlockAllocator *heap = nullptr;
};

namespace process {
    extern Process *activeProcess;
} // process
#endif //MAIN_PROCESS_H