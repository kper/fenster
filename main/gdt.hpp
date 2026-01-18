#ifndef MAIN_GDT_HPP
#define MAIN_GDT_HPP

#include <stdint.h>

struct PrivilegeStackTable
{
    uint64_t s0;
    uint64_t s1;
    uint64_t s2;
} __attribute__((packed));

struct InterruptStackTable
{
    uint64_t s0;
    uint64_t s1;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
} __attribute__((packed));

struct TaskStateSegment
{
    uint32_t reserved0;
    PrivilegeStackTable pst;
    uint64_t reserved1;
    InterruptStackTable ist;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io;
} __attribute__((packed));

struct GdtDescriptor {
    uint16_t limit; // Maximum addressable byte (inclusive bound)
    uint64_t base;  // Base address of the IDT
} __attribute__((packed));

constexpr uint64_t DF_SIZE = 4 * 0x1000;

constexpr uint16_t CS_SELECTOR  = 0x08; // at gdt[1]
constexpr uint16_t TSS_SELECTOR = 0x10; // at gdt[2..3]

class GDT
{
public:
    void init();

private:
    uint64_t gdt[4]; // For now we only need 3 entries: 0 | CS | TSS (2 entries wide)
    TaskStateSegment tss;
    uint8_t df_stack[DF_SIZE];
};

#endif // MAIN_GDT_HPP
