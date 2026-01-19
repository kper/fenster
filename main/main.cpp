#pragma GCC diagnostic ignored "-Wexcessive-regsave"

#include <stdint.h>

#include "ioutils.hpp"
#include "vga.hpp"
#include "pic.hpp"
#include "idt.hpp"
#include "bootinfo.hpp"
#include "keyboard.h"
#include "Process.h"
#include "syscall.h"
#include "memory/memory.h"
#include "memory/virtual/BlockAllocator.h"
#include "paging/paging.h"
#include "x86/regs.h"
#include "usermode.h"
#include "serial.h"

static GDT gdt = GDT();
static IDT idt = IDT();
static ChainedPICs pics = ChainedPICs();

// Global framebuffer info (set in kernel_main, used in kernel_main_high)
static const Multiboot2TagFramebuffer* g_framebuffer = nullptr;

// Prepare exception handlers

void print_stack_frame(InterruptStackFrame *frame) {
    serial::write_string("ExceptionStackFrame {\n");
    serial::write_string("  instruction_pointer: ");
    serial::write_hex(frame->rip);
    serial::write_char('\n');
    serial::write_string("  code_segment: ");
    serial::write_hex(frame->cs);
    serial::write_char('\n');
    serial::write_string("  cpu_flags: ");
    serial::write_hex(frame->rflags);
    serial::write_char('\n');
    serial::write_string("  stack_pointer: ");
    serial::write_hex(frame->rsp);
    serial::write_char('\n');
    serial::write_string("  stack_segment: ");
    serial::write_hex(frame->ss);
    serial::write_char('\n');
    serial::write_string("  page_fault_linear_address: ");
    serial::write_hex(cr2::get_pfla());
    serial::write_char('\n');
    serial::write_string("}\n");
}

__attribute__((interrupt)) void de_handler(InterruptStackFrame *frame)
{
    SERIAL_ERROR("Exception: Division by zero");
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void ud_handler(InterruptStackFrame *frame)
{
    SERIAL_ERROR("Exception: Invalid opcode");
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void df_handler(InterruptStackFrame *frame, uint64_t code)
{
    serial::write_string("[ERROR] Exception: Double Fault [code: ");
    serial::write_dec(code);
    serial::write_string("]\n");
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void pf_handler(InterruptStackFrame *frame, uint64_t code)
{
    serial::write_string("[ERROR] Exception: Page Fault (error code: ");
    serial::write_dec(code);
    serial::write_string(")\n");
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void timer_handler(InterruptStackFrame *frame)
{
    serial::write_char('.');

    pics.notify_end_of_interrupt(Interrupt::TIMER);
}

__attribute__((interrupt)) void keyboard_handler(InterruptStackFrame *frame)
{
    uint8_t scancode = inb(0x60);
    // auto& out = vga::out();
    // VgaFormat before = out.format;
    // out << CYAN << "code: " << scancode << before << out.endl;
    keyboard::addScancode(scancode);
    pics.notify_end_of_interrupt(Interrupt::KEYBOARD);
}

//Process* activeProcess = null;

// The actual syscall handler implementation
extern "C" uint64_t syscall_handler_inner(uint64_t syscall_number, uint64_t syscall_arg)
{
    if (syscall_number != 6) {
        serial::write_string("SYCALL TRIGGERD ");
        serial::write_string("syscall_number: ");
        serial::write_dec(syscall_number);
        serial::write_string(" syscall_arg: ");
        serial::write_dec(syscall_arg);
        serial::write_char('\n');
    }
    switch (syscall_number) {
        case Syscall::NOOP: {
            SERIAL_INFO("[SYSCALL NOOP] Test triggered!");
            return 0;
        }
        case Syscall::WRITE_CHAR: {
            serial::write_char(static_cast<char>(syscall_arg));
            return 0;  // Success
        }
        case Syscall::READ_CHAR: {
            // FIXME: This is bad
            while (!keyboard::hasChar()) {
                // Wait for input
            }
            char c = keyboard::getChar();
            return static_cast<uint64_t>(c);  // Return the character read
        }
        case Syscall::CAN_READ_CHAR: {
            return keyboard::hasChar();
        }
        case Syscall::WRITE: {
            serial::write_string(reinterpret_cast<char *>(syscall_arg));
            return 0;  // Success
        }
        case Syscall::MALLOC: {
            serial::write_string("ALLOC\n");
            auto tmp =  (uint64_t)process::activeProcess->heap->allocate(syscall_arg, 0);
            serial::write_string("ALLOC ENDE\n");
            return tmp;
        }
        case Syscall::FREE: {
            process::activeProcess->heap->deallocate((void*)syscall_arg, 0);
            return 0;
        }
        case Syscall::DRAW: {
            uint32_t width = g_framebuffer->framebuffer_width;
            uint32_t height = g_framebuffer->framebuffer_height;
            uint32_t* framebuffer = g_framebuffer->get_buffer();
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    uint32_t pixel_index = y * width + x;
                    framebuffer[pixel_index] = ((uint32_t*)syscall_arg)[pixel_index];
                }
            }
            return 0;
        }
        case Syscall::GET_SCREEN_WIDTH: {
            return g_framebuffer->framebuffer_width;
        }
        case Syscall::GET_SCREEN_HEIGHT: {
            return g_framebuffer->framebuffer_height;
        }
        case Syscall::EXIT: {
            SERIAL_INFO("[SYSCALL EXIT] TODO");
            return 0;
        }
        default: {
            serial::write_string("[SYSCALL UNKNOWN] System call ");
            serial::write_dec(syscall_number);
            serial::write_char('\n');
            return static_cast<uint64_t>(-1);  // Error
        }
    }
}

// Assembly wrapper that preserves syscall arguments from registers
// Returns the value from syscall_handler_inner in rax
// https://wiki.osdev.org/System_Calls#Interrupts
__attribute__((naked)) void syscall_handler()
{
    asm volatile(
        // Save registers - at entry, rax=syscall_number, rdi=syscall_arg
        "push %%rax\n"           // Save syscall number
        "push %%rbx\n"
        "push %%rcx\n"
        "push %%rdx\n"
        "push %%rsi\n"
        "push %%rdi\n"           // Save syscall arg
        "push %%rbp\n"
        "push %%r8\n"
        "push %%r9\n"
        "push %%r10\n"
        "push %%r11\n"
        "push %%r12\n"
        "push %%r13\n"
        "push %%r14\n"
        "push %%r15\n"

        // Load arguments for C calling convention
        // rdi = first arg (syscall_number), rsi = second arg (syscall_arg)
        "mov 14*8(%%rsp), %%rdi\n"   // Load saved rax (syscall_number)
        "mov 9*8(%%rsp), %%rsi\n"    // Load saved rdi (syscall_arg)

        "call syscall_handler_inner\n"
        // After call, rax contains the return value - we must preserve it!

        // Restore all registers EXCEPT rax (which holds the return value)
        "pop %%r15\n"
        "pop %%r14\n"
        "pop %%r13\n"
        "pop %%r12\n"
        "pop %%r11\n"
        "pop %%r10\n"
        "pop %%r9\n"
        "pop %%r8\n"
        "pop %%rbp\n"
        "pop %%rdi\n"
        "pop %%rsi\n"
        "pop %%rdx\n"
        "pop %%rcx\n"
        "pop %%rbx\n"
        "add $8, %%rsp\n"        // Skip the saved rax (don't restore it, keep return value)
        "iretq\n"
        ::: "memory"
    );
}

extern "C" void kernel_main(void *mb_info_addr)
{
    // Initialize serial port for debugging (do this first!)
    serial::init();
    SERIAL_INFO("===== Kernel starting =====");

    BootInfo* boot_info = static_cast<BootInfo *>(mb_info_addr);

    // Print kernel memory layout from ELF sections
    SERIAL_INFO("Boot information:");
    // Note: boot_info->print() writes to VGA, we'd need to refactor it to use serial
    // For now, just skip it

    // Store framebuffer info for later use
    auto fb = boot_info->get_framebuffer();
    if (fb.has_value()) {
        g_framebuffer = fb.value();
        SERIAL_INFO("Framebuffer available - will test after memory setup");
    }

    // Remap kernel and jump to high addresses
    // This function does NOT return! It jumps to kernel_main_high()
    memory::init_and_jump_high(*boot_info);

    __builtin_unreachable();
}

// This function runs entirely at high addresses
extern "C" void kernel_main_high() {
    SERIAL_INFO("Now running at high addresses!");
    serial::write_string("Kernel address: ");
    serial::write_hex((uint64_t) &kernel_main_high);
    serial::write_char('\n');

    // Update framebuffer pointer to high address (like we do for VGA)
    if (g_framebuffer != nullptr) {
        g_framebuffer = reinterpret_cast<const Multiboot2TagFramebuffer*>(
            reinterpret_cast<uint64_t>(g_framebuffer) + paging::KERNEL_OFFSET
        );
        serial::write_string("Updated framebuffer pointer to: ");
        serial::write_hex((uint64_t)g_framebuffer);
        serial::write_char('\n');
    }

    // Initialize heap BEFORE unmapping lower half
    // This is necessary because placement new for BlockAllocator needs access to constructor code
    SERIAL_INFO("Initializing heap...");
    memory::init_heap();

    // Now it's safe to unmap the lower half
    paging::unmap_lower_half();
    SERIAL_INFO("Lower half unmapped successfully!");

    // Initialize GDT, TSS + IST (once, at high addresses)
    SERIAL_INFO("Initializing GDT...");
    gdt.init();

    // Set up the IDT with all handlers
    SERIAL_INFO("Setting up IDT...");
    idt.set_idt_entry(0, de_handler);
    idt.set_idt_entry(6, ud_handler);
    idt.set_idt_entry_err(14, pf_handler);
    // On #DF, switch to the df-stack (at idx 1)
    idt.set_idt_entry_err(8, 1, df_handler);
    idt.load();

    // Init PIC & register interrupts handlers
    SERIAL_INFO("Initializing PIC...");
    pics.init(PIC_1_OFFSET, PIC_2_OFFSET);
    idt.set_idt_entry(Interrupt::TIMER, timer_handler);
    idt.set_idt_entry(Interrupt::KEYBOARD, keyboard_handler);

    // Register syscall handler at vector 0x80 with DPL=3 (allows ring 3 to call)
    idt.set_idt_entry_user(Interrupt::SYSCALL, reinterpret_cast<void(*)(InterruptStackFrame*)>(syscall_handler));

    // Disable the timer for now
    pics.disable(Interrupt::TIMER);

    interrupts_enable();

    SERIAL_INFO("Kernel setup complete");

    // Test syscall from kernel space
    SERIAL_INFO("Testing syscall mechanism...");
    write_char('O');
    write_char('K');
    write_char('\n');
    SERIAL_INFO("Syscall test complete!");

    SERIAL_INFO("Testing heap memory allocation...");
    int * ptr = (int*) memory::kernel_heap->allocate(sizeof(int), 0);
    serial::write_string("  ptr: ");
    serial::write_hex((uint64_t)ptr);
    serial::write_char('\n');
    *ptr = 123;
    memory::kernel_heap->deallocate(ptr, sizeof(int));
    serial::write_string("  value: ");
    serial::write_dec(*ptr);
    serial::write_char('\n');
    SERIAL_INFO("Heap memory allocation test complete!");

    SERIAL_INFO("We are still alive!");

    // Test framebuffer if available
    if (g_framebuffer != nullptr) {
        SERIAL_INFO("Testing framebuffer...");

        uint64_t fb_addr = g_framebuffer->framebuffer_addr;
        uint32_t width = g_framebuffer->framebuffer_width;
        uint32_t height = g_framebuffer->framebuffer_height;
        uint64_t fb_size = width * height * 4;  // 4 bytes per pixel (32bpp)

        serial::write_string("Framebuffer address: ");
        serial::write_hex(fb_addr);
        serial::write_string(", size: ");
        serial::write_dec(fb_size);
        serial::write_string(" bytes\n");

        // Map framebuffer pages (identity map for simplicity)
        auto page_table = paging::ActivePageTable::instance();
        uint64_t fb_start = fb_addr & ~0xFFF;  // Align down to page boundary
        uint64_t fb_end = (fb_addr + fb_size + 0xFFF) & ~0xFFF;  // Align up

        serial::write_string("Mapping pages from ");
        serial::write_hex(fb_start);
        serial::write_string(" to ");
        serial::write_hex(fb_end);
        serial::write_char('\n');

        for (uint64_t addr = fb_start; addr < fb_end; addr += 0x1000) {
            auto frame = memory::Frame::containing_address(addr);
            page_table.identity_map(frame, paging::PageFlags{.writable = true}, *memory::frame_allocator);
        }

        SERIAL_INFO("Framebuffer mapped! Drawing test pattern...");

        // Now we can safely write to framebuffer
        uint32_t* framebuffer = g_framebuffer->get_buffer();

        // for (uint32_t y = 0; y < height; y++) {
        //     for (uint32_t x = 0; x < width; x++) {
        //         uint32_t pixel_index = y * width + x;
        //         framebuffer[pixel_index] = image_data[y][x];
        //     }
        // }
        SERIAL_INFO("Framebuffer test complete! Screen should be red/blue.");
    } else {
        SERIAL_WARN("No framebuffer available");
    }

    // Jump to ring 3 usermode
    SERIAL_INFO("Jumping to ring 3...");
    gdt.jump_to_ring3(user_function);

    // Infinite loop
    while (true) {
        asm volatile("hlt");
    }
}

void allocation_test(memory::AreaFrameAllocator &allocator) {
    SERIAL_INFO("Allocating all available frames...");

    uint64_t count = 0;
    memory::Frame last_frame;

    while (true) {
        auto frame = allocator.allocate_frame();
        if (frame.is_empty()) {
            // Out of memory (frame 0 with number==0 after allocating others means OOM)
            break;
        }

        last_frame = frame.value();
        count++;

        // Print progress every 1024 frames to avoid too much output
        if (count % 1024 == 0) {
            serial::write_string("  Allocated ");
            serial::write_dec(count);
            serial::write_string(" frames...\n");
        }
    }

    serial::write_string("Total frames allocated: ");
    serial::write_dec(count);
    serial::write_char('\n');
    serial::write_string("Last frame: #");
    serial::write_dec(last_frame.number);
    serial::write_string(" (");
    serial::write_hex(last_frame.start_address());
    serial::write_string(")\n");
    serial::write_string("Total memory allocated: ");
    serial::write_dec(count * memory::PAGE_SIZE);
    serial::write_string(" bytes\n");
}