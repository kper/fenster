ENTRY(_start)

SECTIONS
{
    /* Load address (1MB is typical for x86_64 kernels) */
    . = 1M;

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

    /* Define stack space (16 KiB) */
    .stack (NOLOAD) : ALIGN(16) {
        . = . + 16K;
        _stack_top = .;
    }
}

