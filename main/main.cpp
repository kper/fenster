#pragma GCC diagnostic ignored "-Wexcessive-regsave"

#include <stdint.h>

#include "ioutils.hpp"
#include "vga.hpp"
#include "pic.hpp"
#include "idt.hpp"
#include "bootinfo.hpp"
#include "syscall.h"
#include "memory/memory.h"
#include "paging/paging.h"
#include "x86/regs.h"
#include "usermode.h"

static GDT gdt = GDT();
static IDT idt = IDT();
static ChainedPICs pics = ChainedPICs();

// Prepare exception handlers

void print_stack_frame(InterruptStackFrame *frame) {
    
    VgaFormat orig = vga::out.format;

    vga::out << YELLOW;
    vga::out << "ExceptionStackFrame {" << vga::out.endl;

    vga::out << "  " << "instruction_pointer: " << hex << frame->rip << vga::out.endl;
    vga::out << "  " << "code_segment: " << frame->cs << vga::out.endl;
    vga::out << "  " << "cpu_flags: " << frame->rflags << vga::out.endl;
    vga::out << "  " << "stack_pointer: " << hex << frame->rsp << vga::out.endl;
    vga::out << "  " << "stack_segment: " << frame->ss << vga::out.endl;
    vga::out << "  " << "page_fault_linear_addres: " << hex << cr2::get_pfla() << vga::out.endl;
    vga::out << "}" << vga::out.endl;
    vga::out << orig;
}

__attribute__((interrupt)) void de_handler(InterruptStackFrame *frame)
{
    VgaFormat before = vga::out.format;
    vga::out << YELLOW << "Exception: Division by zero" << before << vga::out.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void ud_handler(InterruptStackFrame *frame)
{
    VgaFormat before = vga::out.format;
    vga::out << YELLOW << "Exception: Invalid opcode" << before << vga::out.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void df_handler(InterruptStackFrame *frame, uint64_t code)
{
    VgaFormat before = vga::out.format;
    vga::out << YELLOW << "Exception: Double Fault" << before << vga::out.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void pf_handler(InterruptStackFrame *frame, uint64_t code)
{
    VgaFormat before = vga::out.format;
    vga::out << YELLOW << "Exception: Page Fault (error code: " << code << ")" << before << vga::out.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void timer_handler(InterruptStackFrame *frame)
{
    vga::out << ".";

    pics.notify_end_of_interrupt(Interrupt::TIMER);
}

__attribute__((interrupt)) void keyboard_handler(InterruptStackFrame *frame)
{
    uint8_t scancode = inb(0x60);

    if ((scancode & 0x80) != 0) {
        // Do nothing on key release
        pics.notify_end_of_interrupt(Interrupt::KEYBOARD);
        return;
    }

    char c = 0;
    // Keyboard scan code to character mapping array
    // Non-printable characters are represented as 0
    char scancode_map[] = {
        0, // 0x00 - no key
        0, // 0x01 - escape pressed
        '1', // 0x02 - 1 pressed
        '2', // 0x03 - 2 pressed
        '3', // 0x04 - 3 pressed
        '4', // 0x05 - 4 pressed
        '5', // 0x06 - 5 pressed
        '6', // 0x07 - 6 pressed
        '7', // 0x08 - 7 pressed
        '8', // 0x09 - 8 pressed
        '9', // 0x0A - 9 pressed
        '0', // 0x0B - 0 (zero) pressed
        '-', // 0x0C - - pressed
        '=', // 0x0D - = pressed
        '\b', // 0x0E - backspace pressed
        '\t', // 0x0F - tab pressed
        'q', // 0x10 - Q pressed
        'w', // 0x11 - W pressed
        'e', // 0x12 - E pressed
        'r', // 0x13 - R pressed
        't', // 0x14 - T pressed
        'y', // 0x15 - Y pressed
        'u', // 0x16 - U pressed
        'i', // 0x17 - I pressed
        'o', // 0x18 - O pressed
        'p', // 0x19 - P pressed
        '[', // 0x1A - [ pressed
        ']', // 0x1B - ] pressed
        '\n', // 0x1C - enter pressed
        0, // 0x1D - left control pressed
        'a', // 0x1E - A pressed
        's', // 0x1F - S pressed
        'd', // 0x20 - D pressed
        'f', // 0x21 - F pressed
        'g', // 0x22 - G pressed
        'h', // 0x23 - H pressed
        'j', // 0x24 - J pressed
        'k', // 0x25 - K pressed
        'l', // 0x26 - L pressed
        ';', // 0x27 - ; pressed
        '\'', // 0x28 - ' (single quote) pressed
        '`', // 0x29 - ` (back tick) pressed
        0, // 0x2A - left shift pressed
        '\\', // 0x2B - \ pressed
        'z', // 0x2C - Z pressed
        'x', // 0x2D - X pressed
        'c', // 0x2E - C pressed
        'v', // 0x2F - V pressed
        'b', // 0x30 - B pressed
        'n', // 0x31 - N pressed
        'm', // 0x32 - M pressed
        ',', // 0x33 - , pressed
        '.', // 0x34 - . pressed
        '/', // 0x35 - / pressed
        0, // 0x36 - right shift pressed
        '*', // 0x37 - (keypad) * pressed
        0, // 0x38 - left alt pressed
        ' ', // 0x39 - space pressed
        0, // 0x3A - CapsLock pressed
        0, // 0x3B - F1 pressed
        0, // 0x3C - F2 pressed
        0, // 0x3D - F3 pressed
        0, // 0x3E - F4 pressed
        0, // 0x3F - F5 pressed
        0, // 0x40 - F6 pressed
        0, // 0x41 - F7 pressed
        0, // 0x42 - F8 pressed
        0, // 0x43 - F9 pressed
        0, // 0x44 - F10 pressed
        0, // 0x45 - NumberLock pressed
        0, // 0x46 - ScrollLock pressed
        '7', // 0x47 - (keypad) 7 pressed
        '8', // 0x48 - (keypad) 8 pressed
        '9', // 0x49 - (keypad) 9 pressed
        '-', // 0x4A - (keypad) - pressed
        '4', // 0x4B - (keypad) 4 pressed
        '5', // 0x4C - (keypad) 5 pressed
        '6', // 0x4D - (keypad) 6 pressed
        '+', // 0x4E - (keypad) + pressed
        '1', // 0x4F - (keypad) 1 pressed
        '2', // 0x50 - (keypad) 2 pressed
        '3', // 0x51 - (keypad) 3 pressed
        '0', // 0x52 - (keypad) 0 pressed
        '.' // 0x53 - (keypad) . pressed
    };

    if (scancode < sizeof(scancode_map)) {
        c = scancode_map[scancode];
    }

    if (c == 0) {
        c = '.';
    }

    vga::out << c;

    pics.notify_end_of_interrupt(Interrupt::KEYBOARD);
}

// The actual syscall handler implementation
extern "C" void syscall_handler_inner(uint64_t syscall_number, uint64_t syscall_arg)
{
    VgaFormat before = vga::out.format;
    vga::out << CYAN << "[SYSCALL ] num: " << syscall_number << " arg: " << syscall_arg << before << vga::out.endl;
    switch (syscall_number) {
        case Syscall::NOOP: {
            vga::out << CYAN << "[SYSCALL NOOP] Test triggered!" << before << vga::out.endl;
            break;
        }
        case Syscall::WRITE_CHAR: {
            vga::out << static_cast<char>(syscall_arg);
            break;
        }
        case Syscall::READ_CHAR: {
            vga::out << CYAN << "[SYSCALL READ_CHAR] TODO" << before << vga::out.endl;
            break;
        }
        case Syscall::EXIT: {
            vga::out << CYAN << "[SYSCALL EXIT] TODO" << before << vga::out.endl;
            break;
        }
        default: {
            vga::out << CYAN << "[SYSCALL UNKNOWN] System " << syscall_number << before << vga::out.endl;
            break;
        }
    }
}

// Assembly wrapper that preserves syscall arguments from registers
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

        // Restore all registers
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
        "pop %%rax\n"
        "iretq\n"
        ::: "memory"
    );
}

extern "C" void kernel_main(void *mb_info_addr)
{
    // Sets up GDT, TSS + IST
    gdt.init();

    // Set up the IDT
    idt.set_idt_entry(0, de_handler);
    idt.set_idt_entry(6, ud_handler);
    idt.set_idt_entry_err(14, pf_handler);
    // On #DF, switch to the df-stack (at idx 1)
    idt.set_idt_entry_err(8, 1, df_handler);
    idt.load();
    
    
    // Init PIC & register interrupts handlers
    pics.init(PIC_1_OFFSET, PIC_2_OFFSET);
    idt.set_idt_entry(Interrupt::TIMER, timer_handler);
    idt.set_idt_entry(Interrupt::KEYBOARD, keyboard_handler);

    // Register syscall handler at vector 0x80
    idt.set_idt_entry(Interrupt::SYSCALL, reinterpret_cast<void(*)(InterruptStackFrame*)>(syscall_handler));

    // Disable the timer for now
    pics.disable(Interrupt::TIMER);
    
    interrupts_enable();


    vga::out.clear();
    vga::out << GREEN << "Kernel setup complete" << WHITE << vga::out.endl;

    BootInfo* boot_info = static_cast<BootInfo *>(mb_info_addr);

    // Print kernel memory layout from ELF sections
    vga::out << vga::out.endl;


    boot_info->print(vga::out);

    using vga::out;
    vga::out << vga::out.endl;

    memory::init(*boot_info);

    // allocator.allocate_frame();

    // Test syscall from kernel space
    out << "Testing syscall mechanism..." << out.endl;
    write_char('O');
    write_char('K');
    write_char('\n');

    out << "Syscall test complete!" << out.endl;

    // Jump to ring 3 usermode
    //extern void user_function();
    out << "Jumping to ring 3..." << out.endl;
    gdt.jump_to_ring3(user_function);

    out << "We are still alive!" << out.endl;
}

void allocation_test(memory::FrameAllocator &allocator) {
    using vga::out;
    out << "Allocating all available frames..." << out.endl;

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
            out << "  Allocated " << count << " frames..." << out.endl;
        }
    }

    out << "Total frames allocated: " << count << out.endl;
    out << "Last frame: #" << last_frame.number << " (" << hex << last_frame.start_address() << ")" << out.endl;
    out << "Total memory allocated: " << size << count * memory::PAGE_SIZE << out.endl;
}