#include "usermode.h"
#include "vga.hpp"
#include "syscall.h"

// User mode test function that runs in ring 3
void user_function() {

    // for (int i = 0; i < 10; ++i) {
    //     //write_char('A');
    //     asm volatile(
    //     "mov $1, %rax\n"
    //     "mov $65, %rdi\n"
    //     "int $0x80\n"
    //     );
    // }
    write("sh> ");


    while (true) {
        while (!can_read_char()) {
        }
        //write("Can read char: ");
        write_char(read_char());
    }

    //while (true) {
        //write_char(read_char());
    //}

    while (true) {
    // Loop forever
        //asm volatile("hlt");
    }
}