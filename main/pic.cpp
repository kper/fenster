#include <stdint.h>
#include "ioutils.hpp"
#include "pic.hpp"

bool PIC::handles_interrupt(uint8_t interrupt_id)
{
    return offset <= interrupt_id && interrupt_id < offset + 8;
}

void PIC::end_of_interrupt()
{
    outb(command_port, CMD_END_OF_INTERRUPT);
}

uint8_t PIC::read_mask()
{
    return inb(data_port);
}

void PIC::write_mask(uint8_t mask)
{
    outb(data_port, mask);
}

void ChainedPICs::wait()
{
    // We need to add a delay between writes to our PICs, especially on
    // older motherboards.  But we don't necessarily have any kind of
    // timers yet, because most of them require interrupts.  Various
    // older versions of Linux and other PC operating systems have
    // worked around this by writing garbage data to port 0x80, which
    // allegedly takes long enough to make everything work on most
    // hardware.
    outb(0x80, 0);
}

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
    outb(pics[0].command_port, CMD_INIT);
    wait();
    outb(pics[1].command_port, CMD_INIT);

    // Byte 1: Set up our base offsets.
    outb(pics[0].data_port, pics[0].offset);
    wait();
    outb(pics[1].data_port, pics[1].offset);
    wait();

    // Byte 2: Configure chaining between PIC1 and PIC2.
    outb(pics[0].data_port, 4);
    wait();
    outb(pics[1].data_port, 2);
    wait();

    // Byte 3: Set our mode.
    outb(pics[0].data_port, MODE_8086);
    wait();
    outb(pics[1].data_port, MODE_8086);
    wait();

    // Restore our saved masks.
    pics[0].write_mask(saved_mask1);
    wait();
    pics[1].write_mask(saved_mask2);
}

// Disables both PICs by masking all interrupts.
void ChainedPICs::disable_all()
{
    pics[0].write_mask(0xFF);
    wait();
    pics[1].write_mask(0xFF);
}

void ChainedPICs::disable(uint8_t interrupt_id)
{
    if (pics[1].handles_interrupt(interrupt_id))
    {
        uint8_t mask = pics[1].read_mask();
        wait();
        pics[1].write_mask(mask | (interrupt_id - pics[1].offset + 1));
    }
    else if (pics[0].handles_interrupt(interrupt_id))
    {
        uint8_t mask = pics[0].read_mask();
        wait();
        pics[0].write_mask(mask | (interrupt_id - pics[0].offset + 1));
    }
}

void ChainedPICs::enable(uint8_t interrupt_id)
{
    if (pics[1].handles_interrupt(interrupt_id))
    {
        uint8_t mask = pics[1].read_mask();
        wait();
        pics[1].write_mask(mask & ~(interrupt_id - pics[1].offset + 1));
    }
    else if (pics[0].handles_interrupt(interrupt_id))
    {
        uint8_t mask = pics[0].read_mask();
        wait();
        pics[0].write_mask(mask & ~(interrupt_id - pics[0].offset + 1));
    }
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
