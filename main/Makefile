all:
	mkdir -p build
	zig cc -target x86_64-freestanding -nostdlib -ffreestanding -Wl -T linker.ld start.S main.cpp -o build/boot.elf

clean:
	rm -rf build

.PHONE: all clean
