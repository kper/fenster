//
// Created by Johannes Zottele on 04.12.25.
//

#include "panic.h"
#include "vga.hpp"

// External VGA stream instance
using vga::out;

[[noreturn]] void panic(const char* message, const char* file, int line) {
    // Disable interrupts to prevent further execution
    asm volatile("cli");

    // Set red background for panic screen
    out << VgaFormat{WHITE, RED};

    // Print panic message
    out << "KERNEL PANIC!\n";
    out << "=============\n\n";
    out << "Message: " << message << "\n";
    out << "File: " << file << "\n";
    out << "Line: " << line << "\n\n";
    out << "System halted.\n";

    // Halt the CPU forever
    while (1) {
        asm volatile("hlt");
    }
}
