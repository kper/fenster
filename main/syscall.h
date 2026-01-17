//
// Created by flo on 17.01.26.

#ifndef MAIN_SYSCALL_H
#define MAIN_SYSCALL_H
#include <stdint.h>

enum Syscall {
    NOOP = 0,
    WRITE_CHAR = 1,
    READ_CHAR = 2,
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

#endif //MAIN_SYSCALL_H