//
// Created by Johannes Zottele on 04.12.25.
//

#include "panic.h"
#include "vga.hpp"

// External VGA stream instance
extern VgaOutStream vga;

[[noreturn]] void panic(const char* message, const char* file, int line) {
    // Disable interrupts to prevent further execution
    asm volatile("cli");

    // Set red background for panic screen
    vga << VgaFormat{WHITE, RED};
    vga.clear();

    // Print panic message
    vga << "KERNEL PANIC!\n";
    vga << "=============\n\n";
    vga << "Message: " << message << "\n";
    vga << "File: " << file << "\n";
    vga << "Line: " << line << "\n\n";
    vga << "System halted.\n";

    // Halt the CPU forever
    while (1) {
        asm volatile("hlt");
    }
}
