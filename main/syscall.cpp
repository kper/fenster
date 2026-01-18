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

char can_read_char() {
    return (bool)(raw_syscall(CAN_READ_CHAR, 0));
}

void write_char(char c) {
    raw_syscall(WRITE_CHAR, (uint64_t)c);
}

void write(char *c) {
    raw_syscall(WRITE, (uint64_t)c);
}

void *malloc(uint64_t size) {
    return raw_syscall(MALLOC, size);
}

void free(void *ptr) {
    raw_syscall(FREE, (uint64_t)ptr);
}

void exit(int code) {
    raw_syscall(EXIT, code);
}
