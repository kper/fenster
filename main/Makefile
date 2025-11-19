build:
	mkdir -p build/isofiles/boot/grub
	cp bootloader/grub.cfg build/isofiles/boot/grub/grub.cfg
	zig cc -target x86_64-freestanding -nostdlib -ffreestanding -fno-sanitize=all -mgeneral-regs-only -T linker.ld bootloader/multiboot_header.S bootloader/boot.S bootloader/start.S main.cpp -o build/isofiles/boot/kernel.bin

mkiso:
	i686-elf-grub-mkrescue -o build/os.iso build/isofiles

boot: clean build mkiso
	qemu-system-x86_64 -cdrom build/os.iso \
		-no-reboot -no-shutdown \
		-d int,guest_errors,cpu_reset

all: build mkiso

clean:
	rm -rf build

.PHONE: all clean
