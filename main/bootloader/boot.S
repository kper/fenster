    .section .text
    .globl _start
    .code32

_start:
    # Call main()
    call kernel_main
    hlt
