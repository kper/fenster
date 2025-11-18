#include "vga.cpp"
#include "idt.cpp"

static VgaOutStream vga = VgaOutStream();
static IDT idt = IDT();

// Prepare exception handlers

__attribute__((no_caller_saved_registers)) void print_stack_frame(InterruptStackFrame *frame) {
    
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
    vga << YELLOW << "Exception: Double Fault (error code: " << code << ")" << before << vga.endl;
    print_stack_frame(frame);
    asm volatile("hlt");
}

extern "C" void kernel_main(void)
{
    vga.clear();

    idt.set_idt_entry(0, de_handler);
    idt.set_idt_entry(6, ud_handler);
    idt.set_idt_entry_err(8, df_handler);
    idt.load();

    vga << "Initialized IDT" << vga.endl;

    // Force a real #DE at runtime
    volatile int zero = 0;
    int i = 5 / zero;
    (void)i;

}