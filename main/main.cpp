#pragma GCC diagnostic ignored "-Wexcessive-regsave"

#include <stdint.h>

#include "ioutils.hpp"
#include "vga.hpp"
#include "pic.hpp"
#include "idt.hpp"
#include "bootinfo.hpp"
#include "paging/c3.h"
#include "memory/frame_allocator.h"
#include "paging/paging.h"

static GDT gdt = GDT();
static IDT idt = IDT();
static ChainedPICs pics = ChainedPICs();

// Prepare exception handlers

void print_stack_frame(InterruptStackFrame *frame) {
    
    VgaFormat orig = vga::out.format;

    vga::out << YELLOW;
    vga::out << "ExceptionStackFrame {" << vga::out.endl;

    vga::out << "  " << "instruction_pointer: " << frame->rip << vga::out.endl;
    vga::out << "  " << "code_segment: " << frame->cs << vga::out.endl;
    vga::out << "  " << "cpu_flags: " << frame->rflags << vga::out.endl;
    vga::out << "  " << "stack_pointer: " << frame->rsp << vga::out.endl;
    vga::out << "  " << "stack_segment: " << frame->ss << vga::out.endl;

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
    
    char c;
    if (scancode >= 0x02 && scancode <= 0x0b) {
        c = '0' + (scancode - 1) % 10;
    } else {
        // TODO: Map the other keys
        c = '.';
    }
    
    vga::out << c;
    
    pics.notify_end_of_interrupt(Interrupt::KEYBOARD);
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

    // Disable the timer for now
    pics.disable(Interrupt::TIMER);
    
    interrupts_enable();

    vga::out.clear();
    vga::out << GREEN << "Kernel setup complete" << WHITE << vga::out.endl;

    BootInfo* boot_info = static_cast<BootInfo *>(mb_info_addr);

    // Print kernel memory layout from ELF sections
    vga::out << vga::out.endl;


    boot_info->print(vga::out);

    vga::out << vga::out.endl;

    vga::out << vga::out.endl;
    auto page_table = paging::ActivePageTable::instance();
    vga::out << "=== Page Table (Recursive) ===" << vga::out.endl;
    page_table.print(vga::out, 1);

    using vga::out;

    auto allocator = memory::AreaFrameAllocator::from_boot_info(*boot_info);
    paging::remap_the_kernel(allocator, *boot_info);
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