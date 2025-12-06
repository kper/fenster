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
        *(.eh_frame*)
        *(.eh_frame_hdr*)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .bss : ALIGN(4K) {
        *(.bss*)
        *(COMMON*)
    }

    /* Page tables: 4 KiB each, initially identity-mapped to 2 MiB huge pages */
    .pagetables (NOLOAD) : ALIGN(4096) {
        p4_table = .;
        . = . + 4096;
        p3_table = .;
        . = . + 4096;
        p2_table = .;
        . = . + 4096;
        p1_table = .; /* Reserve one p1 table which we'll use to map the stack + guard page */
        . = . + 4096;
    }

    /* Define stack space. For it to be mapped to a specific p2 entry, align to 2MiB */
    .stack (NOLOAD) : ALIGN(2M) {
        _stack_guard  = .;         /* guard page (unmapped) */
        . = . + 4K;

        _stack_bot = .;
        . = . + 16K;

        _stack_top = .;
    }

    .debug_abbrev : ALIGN(4K) { *(.debug_abbrev*) }
    .debug_loc : ALIGN(4K) { *(.debug_loc*) }
    .debug_info : ALIGN(4K) { *(.debug_info*) }
    .debug_str : ALIGN(4K) { *(.debug_str*) }
    .comment : ALIGN(4K) { *(.comment*) }
    .debug_line : ALIGN(4K) { *(.debug_line*) }
    .debug_ranges : ALIGN(4K) { *(.debug_ranges*) }
    .debug_aranges : ALIGN(4K) { *(.debug_aranges*) }


    .symtab (NOLOAD) : ALIGN(4K) { *(.symtab*) }
    .strtab : ALIGN(4K) { *(.strtab*) }
    .shstrtab : ALIGN(4K) { *(.shstrtab*) }

}

