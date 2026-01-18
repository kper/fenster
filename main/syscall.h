//
// Created by flo on 17.01.26.

#ifndef MAIN_SYSCALL_H
#define MAIN_SYSCALL_H
#include <stdint.h>

enum Syscall {
    NOOP = 0,
    WRITE_CHAR = 1,
    READ_CHAR = 2,
    WRITE = 3,
    MALLOC = 4,
    FREE = 5,
    CAN_READ_CHAR = 6,
    EXIT = 60,
};

/**
 * A raw syscall function, try to avoid using this function directly.
 * All system calls in fenster have a single number and optionally an additional argument.
 *
 * @param syscall_number
 * @param syscall_arg
 * @return
 */
void* raw_syscall(uint64_t syscall_number, uint64_t syscall_arg);

void write_char(char c);

char read_char();

char can_read_char();

void write(char *c);

void* malloc(uint64_t);

void free(uint64_t);

void exit(int code);

#endif //MAIN_SYSCALL_H