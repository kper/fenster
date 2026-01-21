    .section .multiboot_header,"a"
    .align 8                 # Multiboot2 header must be 8-byte aligned

header_start:
    .long 0xe85250d6                  # magic number (multiboot 2)
    .long 0                           # architecture 0 (protected mode i386)
    .long header_end - header_start   # header length

    # checksum = -(magic + architecture + header_length)
    .long 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    # insert optional multiboot tags here

    # framebuffer tag
    .align 8
framebuffer_tag_start:
    .word 5                  # type = framebuffer
    .word 0                  # flags
    .long framebuffer_tag_end - framebuffer_tag_start  # size
    .long 1920               # width
    .long 1080                # height
    .long 32                 # depth (bits per pixel)
framebuffer_tag_end:

    # required end tag
    .align 8
    .word 0                  # type
    .word 0                  # flags
    .long 8                  # size

header_end:
