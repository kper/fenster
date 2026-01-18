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
constexpr uint16_t USER_CODE_SELECTOR = 0x20 | 3; // at gdt[4], DPL=3
constexpr uint16_t USER_DATA_SELECTOR = 0x28 | 3; // at gdt[5], DPL=3

class GDT
{
public:
    void init();
    void jump_to_ring3(void (*user_function)());

private:
    uint64_t gdt[6]; // null | kernel CS | TSS (2 entries) | user code | user data
    TaskStateSegment tss;
    uint8_t df_stack[DF_SIZE];
    uint8_t kernel_stack[8192]; // Stack for kernel when returning from ring 3
};
