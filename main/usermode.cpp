#include "usermode.h"
#include "syscall.h"
#include "slides.h"

// User mode test function that runs in ring 3

int strcmp(char *s1, char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *) s1 - *(unsigned char *) s2;
}

void shell() {
    write("sh> ");

    char *data = (char *) malloc(1024);
    uint64_t size = 0;

    while (true) {
        while (!can_read_char()) {
        }
        //write("Can read char: ");
        char c = read_char();
        write_char(c);

        if (c == '\n') {
            data[size] = '\0';
            size = 0;

            if (strcmp(data, "scream") == 0) {
                write("AHHHHHH!\n");
            } else {
                write("Unknown command: '");
                write(data);
                write("'\n");
            }

            write("sh> ");
        } else if (size < 1024) {
            data[size++] = c;
        }
    }
}

// A powerpoint (without the power)
void weakpoint() {
    int current_slide = 0;
    int max_slide = 10;

    uint32_t width;
    uint32_t height;
    get_screen_size(&width, &height);

    uint32_t *buffer = (uint32_t *) malloc(width * height * sizeof(uint32_t));

    while (true) {
        // Draw
        if (current_slide == 0) {
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    uint32_t pixel_index = y * width + x;
                    buffer[pixel_index] = image_data[y][x];
                }
            }
        } else {
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    uint32_t pixel_index = y * width + x;
                    buffer[pixel_index] = 0x00000ff;
                }
            }
        }

        // Wait for keyboard input
        while (!can_read_char());
        char c = read_char();
        write_char(c);

        // Update state
        if (c == 'n' && current_slide < max_slide) {
            current_slide++;
        }
        if (c == 'p' && current_slide > 0) {
            current_slide--;
        }
    }
}

void user_function() {
    weakpoint();
}
