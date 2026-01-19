//
// Created by flo on 18.01.26.
//

#include "keyboard.h"

#include "vga.hpp"

namespace keyboard {
    char pendingChars[512] = {};
    uint64_t pendingCharCount = 0;
    bool isCapsLockActive = false;
    bool isShiftActive = false;

    char scancode_map_lowercase[] = {
        0, // 0x00 - no key
        0, // 0x01 - escape pressed
        '1', // 0x02 - 1 pressed
        '2', // 0x03 - 2 pressed
        '3', // 0x04 - 3 pressed
        '4', // 0x05 - 4 pressed
        '5', // 0x06 - 5 pressed
        '6', // 0x07 - 6 pressed
        '7', // 0x08 - 7 pressed
        '8', // 0x09 - 8 pressed
        '9', // 0x0A - 9 pressed
        '0', // 0x0B - 0 (zero) pressed
        '-', // 0x0C - - pressed
        '=', // 0x0D - = pressed
        '\b', // 0x0E - backspace pressed
        '\t', // 0x0F - tab pressed
        'q', // 0x10 - Q pressed
        'w', // 0x11 - W pressed
        'e', // 0x12 - E pressed
        'r', // 0x13 - R pressed
        't', // 0x14 - T pressed
        'y', // 0x15 - Y pressed
        'u', // 0x16 - U pressed
        'i', // 0x17 - I pressed
        'o', // 0x18 - O pressed
        'p', // 0x19 - P pressed
        '[', // 0x1A - [ pressed
        ']', // 0x1B - ] pressed
        '\n', // 0x1C - enter pressed
        0, // 0x1D - left control pressed
        'a', // 0x1E - A pressed
        's', // 0x1F - S pressed
        'd', // 0x20 - D pressed
        'f', // 0x21 - F pressed
        'g', // 0x22 - G pressed
        'h', // 0x23 - H pressed
        'j', // 0x24 - J pressed
        'k', // 0x25 - K pressed
        'l', // 0x26 - L pressed
        ';', // 0x27 - ; pressed
        '\'', // 0x28 - ' (single quote) pressed
        '`', // 0x29 - ` (back tick) pressed
        0, // 0x2A - left shift pressed
        '\\', // 0x2B - \ pressed
        'z', // 0x2C - Z pressed
        'x', // 0x2D - X pressed
        'c', // 0x2E - C pressed
        'v', // 0x2F - V pressed
        'b', // 0x30 - B pressed
        'n', // 0x31 - N pressed
        'm', // 0x32 - M pressed
        ',', // 0x33 - , pressed
        '.', // 0x34 - . pressed
        '/', // 0x35 - / pressed
        0, // 0x36 - right shift pressed
        '*', // 0x37 - (keypad) * pressed
        0, // 0x38 - left alt pressed
        ' ', // 0x39 - space pressed
        0, // 0x3A - CapsLock pressed
        0, // 0x3B - F1 pressed
        0, // 0x3C - F2 pressed
        0, // 0x3D - F3 pressed
        0, // 0x3E - F4 pressed
        0, // 0x3F - F5 pressed
        0, // 0x40 - F6 pressed
        0, // 0x41 - F7 pressed
        0, // 0x42 - F8 pressed
        0, // 0x43 - F9 pressed
        0, // 0x44 - F10 pressed
        0, // 0x45 - NumberLock pressed
        0, // 0x46 - ScrollLock pressed
        '7', // 0x47 - (keypad) 7 pressed
        '8', // 0x48 - (keypad) 8 pressed
        '9', // 0x49 - (keypad) 9 pressed
        '-', // 0x4A - (keypad) - pressed
        '4', // 0x4B - (keypad) 4 pressed
        '5', // 0x4C - (keypad) 5 pressed
        '6', // 0x4D - (keypad) 6 pressed
        '+', // 0x4E - (keypad) + pressed
        '1', // 0x4F - (keypad) 1 pressed
        '2', // 0x50 - (keypad) 2 pressed
        '3', // 0x51 - (keypad) 3 pressed
        '0', // 0x52 - (keypad) 0 pressed
        '.' // 0x53 - (keypad) . pressed
        };

    char scancode_map_uppercase[] = {
        0, // 0x00 - no key
        0, // 0x01 - escape pressed
        '!', // 0x02 - 1 pressed (shift: !)
        '@', // 0x03 - 2 pressed (shift: @)
        '#', // 0x04 - 3 pressed (shift: #)
        '$', // 0x05 - 4 pressed (shift: $)
        '%', // 0x06 - 5 pressed (shift: %)
        '^', // 0x07 - 6 pressed (shift: ^)
        '&', // 0x08 - 7 pressed (shift: &)
        '*', // 0x09 - 8 pressed (shift: *)
        '(', // 0x0A - 9 pressed (shift: ()
        ')', // 0x0B - 0 (zero) pressed (shift: ))
        '_', // 0x0C - - pressed (shift: _)
        '+', // 0x0D - = pressed (shift: +)
        '\b', // 0x0E - backspace pressed
        '\t', // 0x0F - tab pressed
        'Q', // 0x10 - Q pressed
        'W', // 0x11 - W pressed
        'E', // 0x12 - E pressed
        'R', // 0x13 - R pressed
        'T', // 0x14 - T pressed
        'Y', // 0x15 - Y pressed
        'U', // 0x16 - U pressed
        'I', // 0x17 - I pressed
        'O', // 0x18 - O pressed
        'P', // 0x19 - P pressed
        '{', // 0x1A - [ pressed (shift: {)
        '}', // 0x1B - ] pressed (shift: })
        '\n', // 0x1C - enter pressed
        0, // 0x1D - left control pressed
        'A', // 0x1E - A pressed
        'S', // 0x1F - S pressed
        'D', // 0x20 - D pressed
        'F', // 0x21 - F pressed
        'G', // 0x22 - G pressed
        'H', // 0x23 - H pressed
        'J', // 0x24 - J pressed
        'K', // 0x25 - K pressed
        'L', // 0x26 - L pressed
        ':', // 0x27 - ; pressed (shift: :)
        '"', // 0x28 - ' (single quote) pressed (shift: ")
        '~', // 0x29 - ` (back tick) pressed (shift: ~)
        0, // 0x2A - left shift pressed
        '|', // 0x2B - \ pressed (shift: |)
        'Z', // 0x2C - Z pressed
        'X', // 0x2D - X pressed
        'C', // 0x2E - C pressed
        'V', // 0x2F - V pressed
        'B', // 0x30 - B pressed
        'N', // 0x31 - N pressed
        'M', // 0x32 - M pressed
        '<', // 0x33 - , pressed (shift: <)
        '>', // 0x34 - . pressed (shift: >)
        '?', // 0x35 - / pressed (shift: ?)
        0, // 0x36 - right shift pressed
        '*', // 0x37 - (keypad) * pressed
        0, // 0x38 - left alt pressed
        ' ', // 0x39 - space pressed
        0, // 0x3A - CapsLock pressed
        0, // 0x3B - F1 pressed
        0, // 0x3C - F2 pressed
        0, // 0x3D - F3 pressed
        0, // 0x3E - F4 pressed
        0, // 0x3F - F5 pressed
        0, // 0x40 - F6 pressed
        0, // 0x41 - F7 pressed
        0, // 0x42 - F8 pressed
        0, // 0x43 - F9 pressed
        0, // 0x44 - F10 pressed
        0, // 0x45 - NumberLock pressed
        0, // 0x46 - ScrollLock pressed
        '7', // 0x47 - (keypad) 7 pressed
        '8', // 0x48 - (keypad) 8 pressed
        '9', // 0x49 - (keypad) 9 pressed
        '-', // 0x4A - (keypad) - pressed
        '4', // 0x4B - (keypad) 4 pressed
        '5', // 0x4C - (keypad) 5 pressed
        '6', // 0x4D - (keypad) 6 pressed
        '+', // 0x4E - (keypad) + pressed
        '1', // 0x4F - (keypad) 1 pressed
        '2', // 0x50 - (keypad) 2 pressed
        '3', // 0x51 - (keypad) 3 pressed
        '0', // 0x52 - (keypad) 0 pressed
        '.' // 0x53 - (keypad) . pressed
    };

    void addScancode(uint8_t scancode) {
        // Shift and capslock special handling
        switch (scancode) {
            case 58: {
                isCapsLockActive = !isCapsLockActive;
                return;
            }
            case 42:
            case 54: {
                isShiftActive = true;
                return;
            }
            case 170:
            case 182: {
                isShiftActive = false;
                return;
            }
        }

        if ((scancode & 0x80) != 0) {
            // Do nothing on key release
            return;
        }

        char character = '.';
        if (scancode < sizeof(scancode_map_lowercase)) {
            if (isShiftActive || isCapsLockActive) {
                character = scancode_map_uppercase[scancode];
            } else {
                character = scancode_map_lowercase[scancode];
            }
        }

        if (pendingCharCount >= 512) {
            return;
        }

        pendingChars[pendingCharCount] = character;
        pendingCharCount++;
    }

    bool hasChar() {
        return pendingCharCount > 0;
    }

    char getChar() {
        return pendingChars[--pendingCharCount];
    }
} // keyboard