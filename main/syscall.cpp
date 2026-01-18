//
// Created by flo on 17.01.26.
//

#include "syscall.h"

void* raw_syscall(uint64_t syscall_number, uint64_t syscall_arg) {
    uint64_t ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)                          // output: rax
        : "a"(syscall_number), "D"(syscall_arg)  // inputs: rax=syscall_number, rdi=syscall_arg
        : "rcx", "r11", "memory"             // clobbered registers
    );
    return reinterpret_cast<void*>(ret);
}

char read_char() {
    return (char)(uint64_t)(raw_syscall(READ_CHAR, 0));
}

void write_char(char c) {
    raw_syscall(WRITE_CHAR, (uint64_t)c);
}

void exit(int code) {
    raw_syscall(EXIT, code);
}