#include "usermode.h"
#include "vga.hpp"
#include "syscall.h"

// User mode test function that runs in ring 3
void user_function() {

    // //write_char('A');
    // asm volatile(
    // "mov $1, %rax\n"
    // "mov $65, %rdi\n"
    // "int $0x80\n"
    // );
    //
    // // Loop forever
    // while (true) {
    //     //asm volatile("hlt");
    // }
}
