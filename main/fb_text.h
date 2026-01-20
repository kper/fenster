#ifndef FB_TEXT_H
#define FB_TEXT_H

#include <stdint.h>

// Framebuffer text rendering
// Assumes 32bpp RGB framebuffer

struct FbTextState {
    uint32_t* framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t cursor_x;  // In characters
    uint32_t cursor_y;  // In characters
    uint32_t fg_color;
    uint32_t bg_color;
};

// Initialize framebuffer text state
void fb_text_init(FbTextState* state, uint32_t* framebuffer, uint32_t width, uint32_t height);

// Set colors
void fb_text_set_colors(FbTextState* state, uint32_t fg, uint32_t bg);

// Draw a single character at pixel position (x, y)
void fb_text_draw_char(FbTextState* state, uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color);

// Put character at current cursor position and advance cursor
void fb_text_putchar(FbTextState* state, char c);

// Put string at current cursor position
void fb_text_puts(FbTextState* state, const char* str);

// Clear screen
void fb_text_clear(FbTextState* state);

// Set cursor position (in characters)
void fb_text_set_cursor(FbTextState* state, uint32_t x, uint32_t y);

#endif // FB_TEXT_H
