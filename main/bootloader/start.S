    .section .text
    .globl long_mode_start
    .code64

long_mode_start:
    
    # Load 0 into all data segment registers
    movw $0, %ax
    movw %ax, %ss
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Enable SSE (the compiler generates some SSE insns in the ISR plogoue, and we don't want #UD errors)
    mov     %cr0, %rax
    and     $~(1 << 2), %rax        # clear CR0.EM (bit 2)
    or      $(1 << 1), %rax         # set CR0.MP (bit 1)
    mov     %rax, %cr0

    mov     %cr4, %rax
    or      $((1 << 9) | (1 << 10)), %rax   # set OSFXSR (bit 9) and OSXMMEXCPT (bit 10)
    mov     %rax, %cr4

    # Run main()
    movl %ebx, %edi      # ebx holds the grub provided multiboot_information_address. We pass it to the main function.
    call kernel_main
    
    # Never return
    1: 
    hlt
    jmp 1b