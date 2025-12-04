#include "vga.hpp"
#include "ioutils.hpp"

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

VgaOutStream &VgaOutStream::operator<<(uint64_t i)
{
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
    for (int y = 1; y <= height; y++)
    {
        for (int x = 0; x < width * 2; x++)
        {
            vgaBuffer[(y - 1) * (width * 2) + x] = vgaBuffer[y * (width * 2) + x];
        }
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

void VgaOutStream::setCursor(int row, int col)
{
    uint16_t pos = row * width + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);

    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);
}
