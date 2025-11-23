#include <stdint.h>

#include "idt.hpp"

void IDT::set_idt_entry(uint8_t vec, uint8_t ist_vec, void (*handler)(InterruptStackFrame *))
{
    uint64_t addr = (uint64_t)handler;
    IdtEntry &e = idt[vec];

    e.pointer_l = addr & 0xFFFF;
    e.pointer_m = (addr >> 16) & 0xFFFF;
    e.pointer_h = (addr >> 32) & 0xFFFFFFFF;
    
    e.gdt_selector = CS_SELECTOR;
    
    // [0..2] Interrupt Stack Table Index
    // [3..7] Reserved
    // [8] 0: Interrupt Gate, 1: Trap Gate
    // [9..11] must be one
    // [12] must be zero
    // [13..14] Descriptor Privilege Level (DPL)
    // [15]	Present
    e.options = 0b1000111000000000;
    e.options |= ist_vec;
    
    e.reserved = 0;
}

void IDT::set_idt_entry(uint8_t vec, void (*handler)(InterruptStackFrame *))
{
    set_idt_entry(vec, 0, handler);
}

void IDT::set_idt_entry_err(uint8_t vec, void (*handler)(InterruptStackFrame *, uint64_t))
{
    set_idt_entry(vec, (void (*)(InterruptStackFrame *))handler);
}

void IDT::set_idt_entry_err(uint8_t vec, uint8_t ist_vec, void (*handler)(InterruptStackFrame *, uint64_t))
{
    set_idt_entry(vec, ist_vec, (void (*)(InterruptStackFrame *))handler);
}

void IDT::load() {
    
    // Prepare the IDT descriptor
    IdtDescriptor d = {
        static_cast<uint16_t>(sizeof(idt) - 1),
        reinterpret_cast<uint64_t>(&idt[0])
    };

    // Load the IDT via 'lidt' instruction
    asm volatile("lidt %0" : : "m"(d) : "memory");
}
