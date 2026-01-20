#include "usermode.h"
#include "syscall.h"
#include "slides.h"
#include "x86/regs.h"

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

    uint32_t width;
    uint32_t height;
    get_screen_size(&width, &height);

    uint32_t *buffer = (uint32_t *) malloc(width * height * sizeof(uint32_t));
    write("buffer allocated\n");

    while (true) {
        // Draw
        uint32_t index = 0;
        for (uint32_t read_index = 0; index < height * width; read_index++) {
            auto cell = slides[current_slide][read_index];
            auto repeater =  (cell >> 24) + 1;
            auto color = cell & 0x00FFFFFF;
            for (int c = 0; c < repeater; c++) {
                buffer[index++] =  color;
            }
        }
        draw(buffer);

        // Wait for keyboard input
        while (!can_read_char()) {
            //asm volatile("hlt");
        };
        char c = read_char();
        write_char(c);

        // Update state
        if (c == 'n' && current_slide < NUM_SLIDES -1) {
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
