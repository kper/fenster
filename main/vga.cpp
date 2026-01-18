#include "vga.hpp"
#include "ioutils.hpp"

// the global instance of the output stream
VgaOutStream VgaOutStream::instance_;

VgaOutStream& VgaOutStream::instance() {
    return instance_;
}

namespace vga {
    // Return reference to the global instance
    // With PIC (-fPIC), this automatically resolves to the correct address
    // via the Global Offset Table (GOT)
    VgaOutStream& out() {
        return VgaOutStream::instance();
    }
}



VgaOutStream &VgaOutStream::operator<<(const char c)
{
    if (xpos >= width)
    {
        newline();
    }

    if (c == '\n')
    {
        newline();
        setCursor(ypos, xpos);
        return *this;
    }

    vgaBuffer[ypos * width * 2 + (xpos * 2)] = c;

    int formatByte = 0x0;

    formatByte |= format.background << 4;
    formatByte |= format.foreground;

    vgaBuffer[ypos * width * 2 + (xpos * 2) + 1] = formatByte;

    xpos++;

    setCursor(ypos, xpos);
    return *this;
}

VgaOutStream &VgaOutStream::operator<<(const char *txt)
{
    for (int i = 0; txt[i] != '\0'; i++)
    {
        *this << txt[i];
    }
    return *this;
}

VgaOutStream &VgaOutStream::operator<<(int i)
{
    if (i == 0) {
        *this << '0';
        return *this;
    }

    if (i < 0)
    {
        *this << '-';
        i = -i;
    }

    int d = 1;
    int t = i;
    while (t != 0)
    {
        t /= 10;
        d *= 10;
    }

    while (d > 1)
    {
        int digit = (i * 10 / d) % 10;
        *this << static_cast<char>('0' + digit);
        d /= 10;
    }

    return *this;
}

VgaOutStream &VgaOutStream::operator<<(uint32_t i)
{
    if (i == 0) {
        *this << '0';
        return *this;
    }

    uint32_t d = 1;
    uint32_t t = i;
    while (t != 0)
    {
        t /= 10;
        d *= 10;
    }

    while (d > 1)
    {
        uint32_t digit = (i * 10 / d) % 10;
        *this << static_cast<char>('0' + digit);
        d /= 10;
    }

    return *this;
}

VgaOutStream &VgaOutStream::operator<<(const void * ptr) {
    *this << hex << reinterpret_cast<uint64_t>(ptr);
    return *this;
}

VgaOutStream &VgaOutStream::operator<<(uint64_t i)
{
    switch (num_format) {
        case NumFormat::HEX:
            print_hex(i);
            num_format = NumFormat::DEC;  // Reset to decimal after use
            return *this;
        case NumFormat::BIN:
            print_bin(i);
            num_format = NumFormat::DEC;  // Reset to decimal after use
            return *this;
        case NumFormat::SIZE:
            print_size(i);
            num_format = NumFormat::DEC;
            return *this;
        case NumFormat::DEC:
        default:
            break;
    }

    if (i == 0) {
        *this << '0';
        return *this;
    }

    uint64_t d = 1;
    uint64_t t = i;
    while (t != 0)
    {
        t /= 10;
        d *= 10;
    }

    while (d > 1)
    {
        uint64_t digit = (i * 10 / d) % 10;
        *this << static_cast<char>('0' + digit);
        d /= 10;
    }

    return *this;
}

VgaOutStream &VgaOutStream::operator<<(const Color foreground)
{
    format = {foreground, format.background};
    return *this;
}

VgaOutStream &VgaOutStream::operator<<(const VgaFormat f)
{
    format = f;
    return *this;
}

void VgaOutStream::newline()
{
    xpos = 0;
    if (ypos < height - 1)
    {
        ypos++;
        return;
    }
    scroll();
}

void VgaOutStream::scroll()
{
    // Shift rows 1..height-1 up to 0..height-2
    for (int y = 1; y < height; ++y)
    {
        for (int x = 0; x < width * 2; ++x)
        {
            vgaBuffer[(y - 1) * (width * 2) + x] = vgaBuffer[y * (width * 2) + x];
        }
    }

    // Clear last row (row height-1) with spaces and current attribute
    const int row_start = (height - 1) * (width * 2);
    for (int x = 0; x < width; ++x)
    {
        vgaBuffer[row_start + x * 2]     = ' ';
        int attr = (format.background << 4) | (format.foreground & 0x0F);
        vgaBuffer[row_start + x * 2 + 1] = attr;
    }
}

void VgaOutStream::clear()
{
    for (int i = 0; i < (height * width * 2); i += 2)
    {
        vgaBuffer[i] = 0;
    }
    ypos = 0;
    xpos = 0;
    format = { WHITE, BLACK };
}

void VgaOutStream::update_buffer_address(uint64_t new_address)
{
    vgaBuffer = reinterpret_cast<volatile char*>(new_address);
}

void VgaOutStream::setCursor(int row, int col)
{
    uint16_t pos = row * width + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);

    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);
}

VgaOutStream &VgaOutStream::operator<<(VgaOutStream& (*manip)(VgaOutStream&))
{
    return manip(*this);
}

void VgaOutStream::print_hex(uint64_t value)
{
    *this << "0x";

    if (value == 0) {
        *this << '0';
        return;
    }

    // Find first non-zero nibble
    int start_nibble = -1;
    for (int i = 15; i >= 0; i--) {
        if ((value >> (i * 4)) & 0xF) {
            start_nibble = i;
            break;
        }
    }

    // Print hex digits
    for (int i = start_nibble; i >= 0; i--) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        if (nibble < 10) {
            *this << static_cast<char>('0' + nibble);
        } else {
            *this << static_cast<char>('a' + (nibble - 10));
        }
    }
}

void VgaOutStream::print_bin(uint64_t value)
{
    *this << "0b";

    if (value == 0) {
        *this << '0';
        return;
    }

    // Find first non-zero bit
    int start_bit = -1;
    for (int i = 63; i >= 0; i--) {
        if ((value >> i) & 1) {
            start_bit = i;
            break;
        }
    }

    // Print binary digits
    for (int i = start_bit; i >= 0; i--) {
        *this << (char)('0' + ((value >> i) & 1));
    }
}

void VgaOutStream::print_size(uint64_t bytes) {
    uint64_t kb = bytes / 1024;
    uint64_t mb = kb / 1024;
    uint64_t gb = mb / 1024;

    if (gb >= 1) {
        *this << dec << gb << " GB";
    } else if (mb >= 1) {
        *this << dec << mb << " MB";
    } else {
        *this << dec << kb << " KB";
    }
}
