#include <stdint.h>
#include "ioutils.hpp"

// Command sent to begin PIC initialization.
constexpr uint8_t CMD_INIT = 0x11;

// Command sent to acknowledge an interrupt.
const uint8_t CMD_END_OF_INTERRUPT = 0x20;

// The mode in which we want to run our PICs.
const uint8_t MODE_8086 = 0x01;

class PIC
{
public:
    uint8_t offset;
    uint8_t command_port;
    uint8_t data_port;

    PIC() : offset(0), command_port(0), data_port(0) {}
    PIC(uint8_t o, uint16_t cp, uint16_t dp)
        : offset(o), command_port(cp), data_port(dp) {}

    bool handles_interrupt(uint8_t interrupt_id);
    void end_of_interrupt();
    
    uint8_t read_mask();
    void write_mask(uint8_t mask);
};

class ChainedPICs
{
public:
    void init(uint8_t offset1, uint8_t offset2);
    
    void enable(uint8_t interrupt_id);
    void disable(uint8_t interrupt_id);
    void disable_all();

    bool handles_interrupt(uint8_t interrupt_id);
    void notify_end_of_interrupt(uint8_t interrupt_id);

private:
    PIC pics[2];

    void wait();
};

constexpr uint8_t PIC_1_OFFSET = 32;
constexpr uint8_t PIC_2_OFFSET = PIC_1_OFFSET + 8;

enum Interrupt {
    TIMER = PIC_1_OFFSET,
    KEYBOARD = 33,
    SYSCALL = 0x80,
};

// Interrupt utilities

static inline bool interrupts_are_enabled()
{
    uint64_t rflags;
    asm volatile(
        "pushfq\n\t"
        "popq %0"
        : "=r"(rflags)
        :
        : "memory"
    );
    return (rflags & (1UL << 9)) != 0;
}

static inline void interrupts_enable()
{
    // Use `memory` to imitate a lock release. Otherwise, the compiler
    // is free to move reads and writes through this asm block.
    asm volatile("sti" ::: "memory");
}

static inline void interrupts_disable()
{
    // Use `memory` to imitate a lock release. Otherwise, the compiler
    // is free to move reads and writes through this asm block.
    asm volatile("cli" ::: "memory");
}