.global _start
_start:
    # Call main()
    call kernel_main

.hang:
    hlt
    jmp .hang
