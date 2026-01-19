#include "usermode.h"
#include "syscall.h"

// User mode test function that runs in ring 3

int strcmp(char *s1, char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void shell() {
    write("sh> ");

    char* data = (char*)malloc(1024);
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

            if (strcmp(data, "scream")  == 0) {
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

void user_function() {
  shell();
}