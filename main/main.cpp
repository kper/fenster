#pragma GCC diagnostic ignored "-Wexcessive-regsave"

#include <stdint.h>

#include "ioutils.hpp"
#include "vga.hpp"
#include "pic.hpp"
#include "idt.hpp"
#include "bootinfo.hpp"
#include "keyboard.h"
#include "syscall.h"
#include "memory/memory.h"
#include "memory/virtual/BlockAllocator.h"
#include "paging/paging.h"
#include "x86/regs.h"
#include "usermode.h"

static GDT gdt = GDT();
static IDT idt = IDT();
static ChainedPICs pics = ChainedPICs();

// Prepare exception handlers

void print_stack_frame(InterruptStackFrame *frame) {
    auto& out = vga::out();
    VgaFormat orig = out.format;

    out << YELLOW;
    out << "ExceptionStackFrame {" << out.endl;

    out << "  " << "instruction_pointer: " << hex << frame->rip << out.endl;
    out << "  " << "code_segment: " << frame->cs << out.endl;
    out << "  " << "cpu_flags: " << frame->rflags << out.endl;
    out << "  " << "stack_pointer: " << hex << frame->rsp << out.endl;
    out << "  " << "stack_segment: " << frame->ss << out.endl;
    out << "  " << "page_fault_linear_addres: " << hex << cr2::get_pfla() << out.endl;
    out << "}" << out.endl;
    out << orig;
}

__attribute__((interrupt)) void de_handler(InterruptStackFrame *frame)
{
    auto& out = vga::out();
    VgaFormat before = out.format;
    out << YELLOW << "Exception: Division by zero" << before << out.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void ud_handler(InterruptStackFrame *frame)
{
    auto& out = vga::out();
    VgaFormat before = out.format;
    out << YELLOW << "Exception: Invalid opcode" << before << out.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void df_handler(InterruptStackFrame *frame, uint64_t code)
{
    auto& out = vga::out();
    VgaFormat before = out.format;
    out << YELLOW << "Exception: Double Fault [code: "<< code << "]" << before << out.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void pf_handler(InterruptStackFrame *frame, uint64_t code)
{
    auto& out = vga::out();
    VgaFormat before = out.format;
    out << YELLOW << "Exception: Page Fault (error code: " << code << ")" << before << out.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void timer_handler(InterruptStackFrame *frame)
{
    auto& out = vga::out();
    out << ".";

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
    auto& out = vga::out();
    VgaFormat before = out.format;
    // out << CYAN << "[SYSCALL ] num: " << syscall_number << " arg: " << syscall_arg << before << out.endl;
    switch (syscall_number) {
        case Syscall::NOOP: {
            out << CYAN << "[SYSCALL NOOP] Test triggered!" << before << out.endl;
            return 0;
        }
        case Syscall::WRITE_CHAR: {
            out << static_cast<char>(syscall_arg);
            return 0;  // Success
        }
        case Syscall::READ_CHAR: {
            // FIXME: This is bad
            while (!keyboard::hasChar()) {
                //out<<".";
            }
            char c = keyboard::getChar();
            //out << c;
            return static_cast<uint64_t>(c);  // Return the character read
        }
        case Syscall::CAN_READ_CHAR: {
            return keyboard::hasChar();
        }
        case Syscall::WRITE: {
            out << reinterpret_cast<char *>(syscall_arg);
            return 0;  // Success
        }
        // case Syscall::MALLOC {
        //
        // }
        // case Syscall::FREE {
        // }
        case Syscall::EXIT: {
            out << CYAN << "[SYSCALL EXIT] TODO" << before << out.endl;
            return 0;
        }
        default: {
            out << CYAN << "[SYSCALL UNKNOWN] System " << syscall_number << before << out.endl;
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
    auto& out = vga::out();
    out.clear();
    out << GREEN << "Kernel starting..." << WHITE << out.endl;

    BootInfo* boot_info = static_cast<BootInfo *>(mb_info_addr);

    // Print kernel memory layout from ELF sections
    out << out.endl;
    boot_info->print(out);
    out << out.endl;

    // Remap kernel and jump to high addresses
    // This function does NOT return! It jumps to kernel_main_high()
    memory::init_and_jump_high(*boot_info);

    __builtin_unreachable();
}

// This function runs entirely at high addresses
extern "C" void kernel_main_high() {
    auto& out = vga::out();

    // Update VGA buffer to high address
    out.update_buffer_address(0xb8000 + paging::KERNEL_OFFSET);

    out << GREEN << "Now running at high addresses! Proof: " << hex << (uint64_t) &kernel_main_high << WHITE << out.endl;

    // Initialize heap BEFORE unmapping lower half
    // This is necessary because placement new for BlockAllocator needs access to constructor code
    memory::init_heap();

    // Now it's safe to unmap the lower half
    paging::unmap_lower_half();

    out << "Lower half unmapped successfully!" << out.endl;

    // Initialize GDT, TSS + IST (once, at high addresses)
    gdt.init();

    // Set up the IDT with all handlers
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

    // Register syscall handler at vector 0x80 with DPL=3 (allows ring 3 to call)
    idt.set_idt_entry_user(Interrupt::SYSCALL, reinterpret_cast<void(*)(InterruptStackFrame*)>(syscall_handler));

    // Disable the timer for now
    pics.disable(Interrupt::TIMER);

    interrupts_enable();

    out << GREEN << "Kernel setup complete" << WHITE << out.endl;

    // Test syscall from kernel space
    out << "Testing syscall mechanism..." << out.endl;
    write_char('O');
    write_char('K');
    write_char('\n');

    out << "Syscall test complete!" << out.endl;

    out << "Testing heap memory allocation..." << out.endl;
    int * ptr = (int*) memory::kernel_heap->allocate(sizeof(int), 0);
    out << "  ptr: " << ptr << out.endl;
    *ptr = 123;
    memory::kernel_heap->deallocate(ptr, sizeof(int));
    out << "  value: " << *ptr << out.endl;
    out << "Heap memory allocation test complete!" << out.endl;


    VGA_DBG("We are still alive!")

    // Jump to ring 3 usermode
    //extern void user_function();
    out << "Jumping to ring 3... (And scream there)" << out.endl;
    gdt.jump_to_ring3(user_function);

    // Infinite loop
    while (true) {
        asm volatile("hlt");
    }
}

void allocation_test(memory::AreaFrameAllocator &allocator) {
    auto& out = vga::out();
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