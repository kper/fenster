build:
	mkdir -p build/isofiles/boot/grub
	cp bootloader/grub.cfg build/isofiles/boot/grub/grub.cfg
	zig cc -target x86_64-freestanding -nostdlib -ffreestanding -Wl -T linker.ld bootloader/multiboot_header.S bootloader/boot.S main.cpp -o build/isofiles/boot/kernel.bin

mkiso:
	i686-elf-grub-mkrescue -o build/os.iso build/isofiles

boot: clean build mkiso
	qemu-system-x86_64 -cdrom build/os.iso

all: build mkiso

clean:
	rm -rf build

.PHONE: all clean
