#include "fb_text.h"
#include "font.h"

void fb_text_init(FbTextState* state, uint32_t* framebuffer, uint32_t width, uint32_t height) {
    state->framebuffer = framebuffer;
    state->width = width;
    state->height = height;
    state->cursor_x = 0;
    state->cursor_y = 0;
    state->fg_color = 0xFFFFFF;  // White
    state->bg_color = 0x000000;  // Black
}

void fb_text_set_colors(FbTextState* state, uint32_t fg, uint32_t bg) {
    state->fg_color = fg;
    state->bg_color = bg;
}

void fb_text_draw_char(FbTextState* state, uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color) {
    // Get the font glyph for this character
    uint8_t char_index = (uint8_t)c;
    const uint8_t* glyph = &FONT_8X16[char_index * FONT_HEIGHT];

    // Draw each row of the character
    for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t glyph_row = glyph[row];

        // Draw each pixel in the row
        for (uint32_t col = 0; col < FONT_WIDTH; col++) {
            uint32_t pixel_x = x + col;
            uint32_t pixel_y = y + row;

            // Check bounds
            if (pixel_x >= state->width || pixel_y >= state->height) {
                continue;
            }

            // Check if this bit is set (font is MSB first, bit 7 is leftmost pixel)
            bool pixel_set = (glyph_row & (1 << (7 - col))) != 0;

            // Calculate framebuffer index
            uint32_t fb_index = pixel_y * state->width + pixel_x;

            // Set pixel color
            state->framebuffer[fb_index] = pixel_set ? fg_color : bg_color;
        }
    }
}

void fb_text_putchar(FbTextState* state, char c) {
    // Handle special characters
    if (c == '\n') {
        state->cursor_x = 0;
        state->cursor_y++;
        return;
    }

    if (c == '\r') {
        state->cursor_x = 0;
        return;
    }

    if (c == '\t') {
        state->cursor_x = (state->cursor_x + 4) & ~3;  // Tab to next multiple of 4
        if (state->cursor_x >= state->width / FONT_WIDTH) {
            state->cursor_x = 0;
            state->cursor_y++;
        }
        return;
    }

    if (c == '\b') {
        if (state->cursor_x > 0) {
            state->cursor_x--;
        }
        return;
    }

    // Calculate pixel position
    uint32_t pixel_x = state->cursor_x * FONT_WIDTH;
    uint32_t pixel_y = state->cursor_y * FONT_HEIGHT;

    // Draw the character
    fb_text_draw_char(state, pixel_x, pixel_y, c, state->fg_color, state->bg_color);

    // Advance cursor
    state->cursor_x++;

    // Handle line wrap
    if (state->cursor_x >= state->width / FONT_WIDTH) {
        state->cursor_x = 0;
        state->cursor_y++;
    }

    // Handle screen scroll (simple: wrap to top for now)
    if (state->cursor_y >= state->height / FONT_HEIGHT) {
        state->cursor_y = 0;
    }
}

void fb_text_puts(FbTextState* state, const char* str) {
    while (*str) {
        fb_text_putchar(state, *str);
        str++;
    }
}

void fb_text_clear(FbTextState* state) {
    // Fill entire framebuffer with background color
    for (uint32_t i = 0; i < state->width * state->height; i++) {
        state->framebuffer[i] = state->bg_color;
    }

    // Reset cursor
    state->cursor_x = 0;
    state->cursor_y = 0;
}

void fb_text_set_cursor(FbTextState* state, uint32_t x, uint32_t y) {
    state->cursor_x = x;
    state->cursor_y = y;
}
