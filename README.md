# Fenster

Fenster is an university project for the Low-Level-Programming course at TU Wien.

# Compilation

Also you need to install the zig compiler for cross compilation.
Go to "main" and run `make build`. We assume that you want to cross compile from AArch64 to X86_64.

# Creating a boot image

We use GRUB2 as the bootloader for our multiboot2 compliant kernel. To create a boot image, make sure
you have `grub` installed. On MacOS it can be installed with:

```
brew install i686-elf-grub
```

Run the `mkiso` make target to create the ISO boot image.

# Run with QEMU

To boot the os in QEMU, first make sure you have the QEMU x86_64 system installed. QEMU can be installed on MacOS with

```
brew install qemu
```

Now run the `boot` make target like this:

```
make boot
```