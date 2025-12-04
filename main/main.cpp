#pragma GCC diagnostic ignored "-Wexcessive-regsave"

#include <stdint.h>

#include "ioutils.hpp"
#include "vga.hpp"
#include "pic.hpp"
#include "idt.hpp"
#include "bootinfo.hpp"
#include "paging/c3.h"
#include "memory/frame_allocator.h"

static VgaOutStream vga = VgaOutStream();
static GDT gdt = GDT();
static IDT idt = IDT();
static ChainedPICs pics = ChainedPICs();

// Prepare exception handlers

void print_stack_frame(InterruptStackFrame *frame) {
    
    VgaFormat orig = vga.format;

    vga << YELLOW;
    vga << "ExceptionStackFrame {" << vga.endl;

    vga << "  " << "instruction_pointer: " << frame->rip << vga.endl;
    vga << "  " << "code_segment: " << frame->cs << vga.endl;
    vga << "  " << "cpu_flags: " << frame->rflags << vga.endl;
    vga << "  " << "stack_pointer: " << frame->rsp << vga.endl;
    vga << "  " << "stack_segment: " << frame->ss << vga.endl;

    vga << "}" << vga.endl;
    vga << orig;
}

__attribute__((interrupt)) void de_handler(InterruptStackFrame *frame)
{
    VgaFormat before = vga.format;
    vga << YELLOW << "Exception: Division by zero" << before << vga.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void ud_handler(InterruptStackFrame *frame)
{
    VgaFormat before = vga.format;
    vga << YELLOW << "Exception: Invalid opcode" << before << vga.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void df_handler(InterruptStackFrame *frame, uint64_t code)
{
    VgaFormat before = vga.format;
    vga << YELLOW << "Exception: Double Fault" << before << vga.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void pf_handler(InterruptStackFrame *frame, uint64_t code)
{
    VgaFormat before = vga.format;
    vga << YELLOW << "Exception: Page Fault (error code: " << code << ")" << before << vga.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

__attribute__((interrupt)) void timer_handler(InterruptStackFrame *frame)
{
    vga << ".";

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
    
    vga << c;
    
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

    vga.clear();
    vga << GREEN << "Kernel setup complete" << WHITE << vga.endl;

    BootInfo* boot_info = static_cast<BootInfo *>(mb_info_addr);

    // Print kernel memory layout from ELF sections
    vga << vga.endl;
    vga << "=== Kernel Memory Layout ===" << vga.endl;

    if (boot_info->has_elf_sections()) {
        auto elf = boot_info->get_elf_sections();
        uint64_t kernel_start = UINT64_MAX;
        uint64_t kernel_end = 0;

        for (auto section = elf->sections_begin(); section != elf->sections_end(); ++section) {
            if (section->size == 0) continue;
            if (section->addr < kernel_start) kernel_start = section->addr;
            if (section->addr + section->size > kernel_end) kernel_end = section->addr + section->size;
        }

        vga << "Kernel start:    0x" << kernel_start << vga.endl;
        vga << "Kernel end:      0x" << kernel_end << vga.endl;
        vga << "Kernel size:     " << (kernel_end - kernel_start) / 1024 << " KB" << vga.endl;
    }

    vga << "Multiboot info:  0x" << (uint64_t)mb_info_addr << vga.endl;

    AreaFrameAllocator(boot_info->get_memory_map(), 0, 0, 0, 0);

    boot_info->print(vga);

    vga << vga.endl;

    vga << vga.endl;
    vga << "=== P4 Page Table (Recursive) ===" << vga.endl;
    paging::P4Table* p4 = c3::get_virt_p4_table();
    vga << "P4 table address: " << hex << (uint64_t) p4 << vga.endl;
    p4->print(vga, 1);
}