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
    // Clear screen and show prompt
    fb_clear();
    fb_set_colors(0x00FF00, 0x000000);  // Green on black
    fb_puts("Fenster Shell\n");
    fb_set_colors(0xFFFFFF, 0x000000);  // White on black
    fb_puts("sh> ");

    char *data = (char *) malloc(1024);
    uint64_t size = 0;

    while (true) {
        char c = read_char();

        // Handle backspace
        if (c == '\b' || c == 127) {
            if (size > 0) {
                size--;
                write_char('\b');  // Serial echo
                fb_putchar('\b');  // Move cursor back
                fb_putchar(' ');   // Overwrite with space
                fb_putchar('\b');  // Move cursor back again
            }
        } else if (c == '\n') {
            // Echo newline
            write_char(c);
            fb_putchar(c);

            data[size] = '\0';
            size = 0;

            if (strcmp(data, "scream") == 0) {
                fb_puts("AHHHHHH!\n");
            } else if (strcmp(data, "clear") == 0) {
                fb_clear();
            } else if (strcmp(data, "ls") == 0) {
                fb_puts("Available programs:\n");
                const char** programs = list_programs();
                for (int i = 0; programs[i] != nullptr; i++) {
                    fb_puts("  - ");
                    fb_puts(programs[i]);
                    fb_puts("\n");
                }
            } else if (strcmp(data, "sl") == 0) {
                fb_puts(" \
                  ooOOOO\n\
                oo      _____\n\
               _I__n_n__||_|| ________\n\
             >(_________|_7_|-|______|\n\
              /o ()() ()() o   oo  oo\n");
            } else if (strcmp(data, "help") == 0) {
                fb_puts("Shell Help:\n\
    run - Run a program by name\n\
    ls - List available programs\n\
    clear - Clear the screen\n\
    scream - Display frustration for os devs\n");

            } else if (strcmp(data, "") == 0) {
                // Empty command, do nothing
            } else {
                // Check if it's a run command
                char run_prefix[] = "run ";
                bool is_run = true;
                for (int i = 0; i < 4; i++) {
                    if (data[i] != run_prefix[i]) {
                        is_run = false;
                        break;
                    }
                }

                if (is_run && data[4] != '\0') {
                    // Extract program name (skip "run ")
                    char* program_name = data + 4;
                    run_program(program_name);
                    // NEVER REACHES HERE - process is replaced
                } else {
                    fb_puts("Unknown command: '");
                    fb_puts(data);
                    fb_puts("'\n");
                    fb_puts("Type 'help' for a list of commands\n");
                }
            }

            fb_puts("sh> ");
        } else if (size < 1024) {
            // Echo character to both serial and framebuffer
            write_char(c);
            fb_putchar(c);
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

        char c = read_char();
        write_char(c);

        // Update state
        if (c == 'n' && current_slide < NUM_SLIDES -1) {
            current_slide++;
        }
        if (c == 'p' && current_slide > 0) {
            current_slide--;
        }
        if (c == 'q') {
            // Exit the program - will never return
            exit(0);
        }
    }
}

void user_function() {
    shell();
}
