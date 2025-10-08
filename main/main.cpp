extern "C" void kernel_main(void) {
    volatile char *vga = (volatile char*)0xB8000;
    vga[0] = 'O'; vga[1] = 0x07;
    vga[2] = 'K'; vga[3] = 0x07;
}
