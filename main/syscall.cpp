//
// Created by flo on 17.01.26.
//

#include "syscall.h"

void* raw_syscall(uint64_t syscall_number, uint64_t syscall_arg) {
    uint64_t ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)                                 // output: rax
        : "a"(syscall_number), "D"(syscall_arg)     // inputs: rax=syscall_number, rdi=syscall_arg
        : "rcx", "r11", "memory"                    // clobbered registers
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

void draw(uint32_t* buffer) {
    raw_syscall(DRAW, (uint64_t)buffer);
}

void get_screen_size(uint32_t* width, uint32_t* height) {
    *width = (uint64_t)raw_syscall(GET_SCREEN_WIDTH, 0);
    *height = (uint64_t)raw_syscall(GET_SCREEN_HEIGHT, 0);
}

void exit(int code) {
    raw_syscall(EXIT, code);
}

void fb_putchar(char c) {
    raw_syscall(FB_PUTCHAR, (uint64_t)c);
}

void fb_puts(const char* str) {
    raw_syscall(FB_PUTS, (uint64_t)str);
}

void fb_clear() {
    raw_syscall(FB_CLEAR, 0);
}

void fb_set_cursor(uint32_t x, uint32_t y) {
    uint64_t packed = ((uint64_t)x << 32) | (uint64_t)y;
    raw_syscall(FB_SET_CURSOR, packed);
}

void fb_set_colors(uint32_t fg, uint32_t bg) {
    uint64_t packed = ((uint64_t)fg << 32) | (uint64_t)bg;
    raw_syscall(FB_SET_COLORS, packed);
}

const char** list_programs() {
    return reinterpret_cast<const char**>(raw_syscall(LIST_PROGRAMS, 0));
}

void run_program(const char* name) {
    raw_syscall(RUN_PROGRAM, (uint64_t)name);
}
