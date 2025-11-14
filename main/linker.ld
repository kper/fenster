ENTRY(_start)

SECTIONS
{
    /* Load address (1MB is typical for x86_64 kernels) */
    . = 1M;

    .boot :
    {
        /* ensure that the multiboot header is at the beginning */
        *(.multiboot_header)
    }

    .text : ALIGN(4K) {
        *(.text*)
    }

    .rodata : ALIGN(4K) {
        *(.rodata*)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .bss : ALIGN(4K) {
        *(.bss*)
        *(COMMON)
    }

    /* Page tables: 4 KiB each, initially identity-mapped to 2 MiB huge pages */
    .pagetables (NOLOAD) : ALIGN(4096) {
        p4_table = .;
        . = . + 4096;
        p3_table = .;
        . = . + 4096;
        p2_table = .;
        . = . + 4096;
    }

    /* Define stack space (16 KiB) */
    .stack (NOLOAD) : ALIGN(16) {
        . = . + 16K;
        _stack_top = .;
    }
}

