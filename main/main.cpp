volatile char *vga = (volatile char*)0xB8000;

void print(const char* txt) {
    for (int i = 0; txt[i] != '\0'; ++i) {
        vga[i * 2]     = txt[i];
        vga[i * 2 + 1] = 0x0b;
    }
}

extern "C" void kernel_main(void) {
    const char* txt = "Welcome to Fenster!";
    print(txt);
}