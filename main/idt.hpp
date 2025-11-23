#include <stdint.h>
#include "gdt.hpp"

struct InterruptStackFrame
{
    uint64_t rip;    // Instruction pointer at time of interrupt
    uint64_t cs;     // Code segment
    uint64_t rflags; // Saved RFLAGS
    uint64_t rsp;    // Stack pointer before the interrupt
    uint64_t ss;     // Stack segment before the interrupt
} __attribute__((packed));

struct IdtEntry
{
    uint16_t pointer_l;     // Function Pointer [0:15]
    uint16_t gdt_selector;  // GDT selector
    uint16_t options;       // Options
    uint16_t pointer_m;     // Function Pointer [16:31]
    uint32_t pointer_h;     // Function Pointer [32:63]
    uint32_t reserved;      // Reserved
} __attribute__((packed));

struct IdtDescriptor
{
    uint16_t limit; // Maximum addressable byte (inclusive bound)
    uint64_t base;  // Base address of the IDT
} __attribute__((packed));

class IDT
{
public:
    
    void set_idt_entry(uint8_t vec, void (*handler)(InterruptStackFrame *));
    void set_idt_entry_err(uint8_t vec, void (*handler)(InterruptStackFrame *, uint64_t));
    
    void set_idt_entry(uint8_t vec, uint8_t ist_vec, void (*handler)(InterruptStackFrame *));
    void set_idt_entry_err(uint8_t vec, uint8_t ist_vec, void (*handler)(InterruptStackFrame *, uint64_t));
    
    void load();

private:
    IdtEntry idt[256];
};