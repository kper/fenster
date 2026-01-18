//
// Created by flo on 18.01.26.
//

#ifndef MAIN_KEYBOARD_H
#define MAIN_KEYBOARD_H
#include <stdint.h>

namespace keyboard {
    void addScancode(uint8_t scancode);

    bool hasChar();

    char getChar();
} // keyboard

#endif //MAIN_KEYBOARD_H