#include <stdint.h>

static inline void out(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

static inline uint8_t in(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

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

    bool handles_interrupt(uint8_t interrupt_id)
    {
        return offset <= interrupt_id && interrupt_id < offset + 8;
    }

    void end_of_interrupt()
    {
        out(command_port, CMD_END_OF_INTERRUPT);
    }

    uint8_t read_mask()
    {
        return in(data_port);
    }

    void write_mask(uint8_t mask)
    {
        out(data_port, mask);
    }
};

class ChainedPICs
{
public:
    void init(uint8_t offset1, uint8_t offset2);
    void disable();

    bool handles_interrupt(uint8_t interrupt_id);
    void notify_end_of_interrupt(uint8_t interrupt_id);

private:
    PIC pics[2];

    void wait()
    {
        // We need to add a delay between writes to our PICs, especially on
        // older motherboards.  But we don't necessarily have any kind of
        // timers yet, because most of them require interrupts.  Various
        // older versions of Linux and other PC operating systems have
        // worked around this by writing garbage data to port 0x80, which
        // allegedly takes long enough to make everything work on most
        // hardware.
        out(0x80, 0);
    }
};

void ChainedPICs::init(uint8_t offset1, uint8_t offset2)
{
    pics[0] = PIC(offset1, 0x20, 0x21);
    pics[1] = PIC(offset2, 0xA0, 0xA1);

    // Save our original interrupt masks, because I'm too lazy to
    // figure out reasonable values.  We'll restore these when we're
    // done.
    uint8_t saved_mask1 = pics[0].read_mask();
    wait();
    uint8_t saved_mask2 = pics[1].read_mask();

    // Tell each PIC that we're going to send it a three-byte
    // initialization sequence on its data port.
    out(pics[0].command_port, CMD_INIT);
    wait();
    out(pics[1].command_port, CMD_INIT);

    // Byte 1: Set up our base offsets.
    out(pics[0].data_port, pics[0].offset);
    wait();
    out(pics[1].data_port, pics[1].offset);
    wait();

    // Byte 2: Configure chaining between PIC1 and PIC2.
    out(pics[0].data_port, 4);
    wait();
    out(pics[1].data_port, 2);
    wait();

    // Byte 3: Set our mode.
    out(pics[0].data_port, MODE_8086);
    wait();
    out(pics[1].data_port, MODE_8086);
    wait();

    // Restore our saved masks.
    pics[0].write_mask(saved_mask1);
    wait();
    pics[1].write_mask(saved_mask2);
}

// Disables both PICs by masking all interrupts.
void ChainedPICs::disable()
{
    pics[0].write_mask(0xFF);
    wait();
    pics[1].write_mask(0xFF);
}

bool ChainedPICs::handles_interrupt(uint8_t interrupt_id)
{
    return pics[0].handles_interrupt(interrupt_id) || pics[1].handles_interrupt(interrupt_id);
}

void ChainedPICs::notify_end_of_interrupt(uint8_t interrupt_id)
{
    if (pics[1].handles_interrupt(interrupt_id))
    {
        pics[1].end_of_interrupt();
    }
    else if (pics[0].handles_interrupt(interrupt_id))
    {
        pics[0].end_of_interrupt();
    }
}

constexpr uint8_t PIC_1_OFFSET = 32;
constexpr uint8_t PIC_2_OFFSET = PIC_1_OFFSET + 8;

enum Interrupt {
    TIMER = PIC_1_OFFSET
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