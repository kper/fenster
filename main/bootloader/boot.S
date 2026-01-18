    .section .text
    .globl _start
    .code32

_start:

    # Init stack pointer
    movl $_stack_top, %esp

    # Preserve ebx (contains GRUB multiboot info pointer)
    pushl %ebx

    # Some startup checks to determine if 64-bit mode is supported
    call check_multiboot
    call check_cpuid
    call check_long_mode

    # Set-up and enable paging
    call set_up_page_tables
    call enable_paging

    # Restore ebx before entering long mode
    popl %ebx

    # Enter 64-bit mode
    lgdt gdt64_pointer                  # Load 64-bit GDT
    # Long-jump to 64-bit code segment (start.S)
    ljmp $gdt64_code, $long_mode_start


# Prints ERR: and the given error code to screen and hangs.
# parameter: error code (in ascii) in al
error:
    movl $0x4f524f45, 0xb8000
    movl $0x4f3a4f52, 0xb8004
    movl $0x4f204f20, 0xb8008
    movb %al, 0xb800a
    hlt

check_multiboot:
    cmpl $0x36d76289, %eax
    jne .no_multiboot
    ret
.no_multiboot:
    movb $'0', %al
    jmp error

check_cpuid:
    # Check if CPUID is supported by attempting to flip the ID bit (bit 21)
    # in the FLAGS register. If we can flip it, CPUID is available.

    # Copy FLAGS in to EAX via stack
    pushfl
    popl %eax

    # Copy to ECX as well for comparing later on
    movl %eax, %ecx

    # Flip the ID bit
    xorl $(1 << 21), %eax

    # Copy EAX to FLAGS via the stack
    pushl %eax
    popfl

    # Copy FLAGS back to EAX (with the flipped bit if CPUID is supported)
    pushfl
    popl %eax

    # Restore FLAGS from the old version stored in ECX (i.e. flipping the
    # ID bit back if it was ever flipped).
    pushl %ecx
    popfl

    # Compare EAX and ECX. If they are equal then that means the bit
    # wasn't flipped, and CPUID isn't supported.
    cmpl %ecx, %eax
    je .no_cpuid
    ret
.no_cpuid:
    movb $'1', %al
    jmp error

check_long_mode:
    # test if extended processor info in available
    movl $0x80000000, %eax  # implicit argument for cpuid
    cpuid                   # get highest supported argument
    cmpl $0x80000001, %eax  # it needs to be at least 0x80000001
    jb  .no_long_mode       # if it's less, the CPU is too old for long mode

    # use extended info to test if long mode is available
    movl $0x80000001, %eax  # argument for extended processor info
    cpuid                   # returns various feature bits in ecx and edx
    testl $(1 << 29), %edx  # test if the LM-bit is set in the D-register
    jz .no_long_mode        # If it's not set, there is no long mode
    ret
.no_long_mode:
    movb $'2', %al
    jmp error


set_up_page_tables:
    # recursive map P4 last entry to P4
    movl    $p4_table, %eax
    orl     $0b111, %eax            # present + writable + user
    movl    %eax, p4_table + 511*8

    # map first P4 entry to P3 table
    movl $p3_table, %eax
    orl $7, %eax         # present + writable + user
    movl %eax, p4_table

    # map first P3 entry to P2 table
    movl $p2_table, %eax
    orl $7, %eax         # present + writable + user
    movl %eax, p3_table

    # map each P2 entry to a huge 2MiB page
    movl $0, %ecx        # counter variable

.map_p2_table:
    # map ecx-th P2 entry to a huge page that starts at address 2MiB*ecx
    movl $0x200000, %eax            # 2MiB
    mull %ecx                       # start address of ecx-th page
    orl $0b10000111, %eax           # present + writable + user + huge
    movl %eax, p2_table(,%ecx,8)    # map ecx-th entry

    incl %ecx           # increase counter
    cmpl $512, %ecx     # if counter == 512, the whole P2 table is mapped
    jne .map_p2_table   # else map the next entry

    # remap the huge page containing the stack explicitly in the single p1_table
    
    movl $_stack_guard, %eax
    shrl $21, %eax                 # index into p2_table (we can do that because the stack is aligned to 2MiB)
    movl %eax, %edi                # save index in EDI

    movl $p1_table, %eax
    orl  $0b111, %eax               # present + writable + user, (not huge)
    movl %eax, p2_table(,%edi,8)    # overwrite the P2 entry containing the stack

    # leave the first P1 entry unmapped (the guard page)
    movl $0, p1_table               # unmapped (no need to set an address if unmapped)

    # identity map remaining P1 entries to 4KiB pages
    movl $1, %ecx

.map_p1_table:
    # map ecx-th P1 entry to a page that starts at address (_stack_guard + 4KiB*ecx)
    movl $0x1000, %eax              # 4KiB
    mull %ecx                       # start address of ecx-th page
    addl $_stack_guard, %eax        # add the _stack_guard offset
    orl $0b111, %eax                # present + writable + user, (not huge)
    movl %eax, p1_table(,%ecx,8)    # map ecx-th entry

    incl %ecx           # increase counter
    cmpl $512, %ecx     # if counter == 512, the whole P1 table is mapped
    jne .map_p1_table   # else map the next entry


    ret

enable_paging:
    # load P4 to cr3 register (cpu uses this to access the P4 table)
    movl $p4_table, %eax
    movl %eax, %cr3

    # enable PAE-flag in cr4 (Physical Address Extension)
    movl %cr4, %eax
    orl $(1 << 5), %eax
    movl %eax, %cr4

    # set the long mode bit in the EFER MSR (model specific register)
    movl $0xC0000080, %ecx
    rdmsr
    orl $(1 << 8), %eax
    wrmsr

    # enable paging in the cr0 register
    movl %cr0, %eax
    orl $(1 << 31), %eax
    movl %eax, %cr0

    ret


# Initial Global Descriptor Table (GDT), will be remapped at kernel startup

    .section .rodata
gdt64:
    .quad 0 # zero entry
gdt64_code = . - gdt64
    .quad (1<<43) | (1<<44) | (1<<47) | (1<<53) # code segment
gdt64_pointer:
    .word . - gdt64 - 1
    .long gdt64