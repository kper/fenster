###############################
# Toolchain & flags
###############################

TARGET_ARCH := x86_64-freestanding
BUILD_DIR   := build
ISO_DIR     := $(BUILD_DIR)/isofiles
BOOT_DIR    := $(ISO_DIR)/boot
OBJ_DIR     := $(BUILD_DIR)/obj
.DEFAULT_GOAL := all

# Compiler selection
# Hint for CLion: when CLION_IDE is set, use absolute system compilers so CLion
# doesn't try to find a file named "cc" in the project directory.
ifdef CLION_IDE
CC  := /usr/bin/clang
CXX := /usr/bin/clang++
AS  := /usr/bin/clang
else
CC  := zig cc
CXX := zig c++
AS  := zig cc
endif

DEBUG    := -g # can be set to -g for debug symbols
CFLAGS   := -target $(TARGET_ARCH) -ffreestanding -nostdlib -fno-sanitize=all -mgeneral-regs-only \
			-Wall -Wextra -Wno-unused-parameter -fPIC
# Preprocessor flags (include current dir); extend as needed
CPPFLAGS := -I .
CXXFLAGS := $(CFLAGS) -std=gnu++17 -fno-exceptions -fno-rtti -O1 $(DEBUG)
ASFLAGS  := -target $(TARGET_ARCH) -ffreestanding -nostdlib -fno-sanitize=all -mgeneral-regs-only $(DEBUG)
LDFLAGS  := -target $(TARGET_ARCH) -nostdlib -ffreestanding -fno-sanitize=all -T linker.ld $(DEBUG)

###############################
# Sources & objects
###############################

SOURCES_CPP := \
    pic.cpp \
    vga.cpp \
    keyboard.cpp \
    gdt.cpp \
    idt.cpp \
    bootinfo.cpp \
    serial.cpp \
    paging/Entry.cpp \
    paging/Table.cpp \
    paging/paging.cpp \
    memory/memory.cpp \
    memory/AreaFrameIterator.cpp \
    memory/frame_allocator.cpp \
    memory/virtual/BumpAllocator.cpp \
    memory/virtual/LinkedListAllocator.cpp \
    memory/virtual/BlockAllocator.cpp \
    runtime/runtime.cpp \
    panic.cpp \
    syscall.cpp \
    usermode.cpp \
    Process.cpp \
    main.cpp

SOURCES_ASM := \
    bootloader/multiboot_header.S \
    bootloader/boot.S \
    bootloader/start.S

OBJECTS := \
    $(SOURCES_CPP:%.cpp=$(OBJ_DIR)/%.o) \
    $(SOURCES_ASM:%.S=$(OBJ_DIR)/%.o)

###############################
# Top-level targets
###############################

.PHONY: all kernel mkiso boot clean prep debug

all: kernel mkiso

# Build only the kernel binary and prepare isofiles
kernel: prep $(BOOT_DIR)/kernel.bin

# Create directories and copy grub config
prep:
	mkdir -p $(BOOT_DIR)/grub
	cp bootloader/grub.cfg $(BOOT_DIR)/grub/grub.cfg

# ISO creation
mkiso: kernel
	i686-elf-grub-mkrescue -o $(BUILD_DIR)/os.iso $(ISO_DIR)

# Run in QEMU
boot: clean kernel mkiso
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/os.iso \
		-no-reboot -no-shutdown \
		-d int,guest_errors,cpu_reset \
		-serial stdio

debug: clean kernel mkiso
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/os.iso \
		-no-reboot -no-shutdown \
		-d int,guest_errors,cpu_reset \
		-serial stdio \
		-S -s

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

###############################
# Build rules (per-file compilation for CLion Makefile sync)
###############################

# Link kernel
$(BOOT_DIR)/kernel.bin: $(OBJECTS) linker.ld
	mkdir -p $(BOOT_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# C++ sources
$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Assembly sources
$(OBJ_DIR)/%.o: %.S
	mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@
