#ifndef MAIN_SERIAL_H
#define MAIN_SERIAL_H

#include <stdint.h>


/**
 * Serial port driver for QEMU debugging
 * COM1 port (0x3F8) - output appears in QEMU's stdout/stderr
 */
namespace serial {
    constexpr uint16_t COM1 = 0x3F8;

    // Initialize serial port
    void init();

    // Write a single character to serial
    void write_char(char c);

    // Write a null-terminated string to serial
    void write_string(const char* str);

    // Write a string with length
    void write(const char* str, uint64_t len);

    // Write an integer in decimal
    void write_dec(uint64_t value);

    // Write an integer in hexadecimal
    void write_hex(uint64_t value);
}

// Debug macros for serial output
#define SERIAL_DEBUG(msg) do { \
    serial::write_string("[DEBUG] "); \
    serial::write_string(__FILE__); \
    serial::write_char(':'); \
    serial::write_dec(__LINE__); \
    serial::write_string(" - "); \
    serial::write_string(msg); \
    serial::write_char('\n'); \
} while(0)

#define SERIAL_INFO(msg) do { \
    serial::write_string("[INFO] "); \
    serial::write_string(msg); \
    serial::write_char('\n'); \
} while(0)

#define SERIAL_WARN(msg) do { \
    serial::write_string("[WARN] "); \
    serial::write_string(msg); \
    serial::write_char('\n'); \
} while(0)

#define SERIAL_ERROR(msg) do { \
    serial::write_string("[ERROR] "); \
    serial::write_string(msg); \
    serial::write_char('\n'); \
} while(0)

#endif // MAIN_SERIAL_H
