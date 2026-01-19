#include "serial.h"
#include "ioutils.hpp"

namespace serial {

// Serial port registers (offsets from base port)
constexpr uint16_t DATA_REG = 0;      // Data register (read/write)
constexpr uint16_t INT_ENABLE = 1;    // Interrupt enable
constexpr uint16_t FIFO_CTRL = 2;     // FIFO control
constexpr uint16_t LINE_CTRL = 3;     // Line control
constexpr uint16_t MODEM_CTRL = 4;    // Modem control
constexpr uint16_t LINE_STATUS = 5;   // Line status

// Line status register bits
constexpr uint8_t TX_EMPTY = 0x20;    // Transmitter holding register empty

void init() {
    // Disable interrupts
    outb(COM1 + INT_ENABLE, 0x00);

    // Enable DLAB (set baud rate divisor)
    outb(COM1 + LINE_CTRL, 0x80);

    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1 + DATA_REG, 0x03);
    outb(COM1 + INT_ENABLE, 0x00);  // (hi byte)

    // 8 bits, no parity, one stop bit
    outb(COM1 + LINE_CTRL, 0x03);

    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + FIFO_CTRL, 0xC7);

    // IRQs enabled, RTS/DSR set
    outb(COM1 + MODEM_CTRL, 0x0B);
}

static bool is_transmit_empty() {
    return inb(COM1 + LINE_STATUS) & TX_EMPTY;
}

void write_char(char c) {
    // Wait for transmit buffer to be empty
    while (!is_transmit_empty());

    // Send character
    outb(COM1 + DATA_REG, c);
}

void write_string(const char* str) {
    if (!str) return;

    while (*str) {
        write_char(*str);
        str++;
    }
}

void write(const char* str, uint64_t len) {
    if (!str) return;

    for (uint64_t i = 0; i < len; i++) {
        write_char(str[i]);
    }
}

void write_dec(uint64_t value) {
    if (value == 0) {
        write_char('0');
        return;
    }

    char buffer[20];  // Enough for 64-bit number
    int i = 0;

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    // Print in reverse order
    while (i > 0) {
        write_char(buffer[--i]);
    }
}

void write_hex(uint64_t value) {
    write_string("0x");

    const char* hex_digits = "0123456789abcdef";
    char buffer[16];

    if (value == 0) {
        write_char('0');
        return;
    }

    int i = 0;
    while (value > 0) {
        buffer[i++] = hex_digits[value & 0xF];
        value >>= 4;
    }

    // Print in reverse order
    while (i > 0) {
        write_char(buffer[--i]);
    }
}

} // namespace serial
