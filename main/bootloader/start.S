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

    # Run main()
    call kernel_main
    hlt