//
// Created by Johannes Zottele on 04.12.25.
//

#ifndef MAIN_PANIC_H
#define MAIN_PANIC_H

/**
 * Panic and halt the system
 * Prints an error message and stops execution
 *
 * @param message Error message to display
 * @param file Source file where panic occurred
 * @param line Line number where panic occurred
 */
[[noreturn]] void panic(const char* message, const char* file, int line);

/**
 * Assert macro that panics if condition is false
 * Usage: ASSERT(condition, "error message")
 */
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            panic(message, __FILE__, __LINE__); \
        } \
    } while (0)

/**
 * Panic macro for direct use
 * Usage: PANIC("error message")
 */
#define PANIC(message) panic(message, __FILE__, __LINE__)

#endif //MAIN_PANIC_H
