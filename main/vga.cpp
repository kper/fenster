enum Color
{
    BLACK = 0x0,
    BLUE = 0x1,
    GREEN = 0x2,
    CYAN = 0x3,
    RED = 0x4,
    MAGENTA = 0x5,
    BROWN = 0x6,
    LIGHT_GRAY = 0x7,
    DARK_GRAY = 0x8,
    LIGHT_BLUE = 0x9,
    LIGHT_GREEN = 0xa,
    LIGHT_CYAN = 0xb,
    LIGHT_RED = 0xc,
    PINK = 0xd,
    YELLOW = 0xe,
    WHITE = 0xf
};

struct VgaFormat
{
    Color foreground;
    Color background;
};

class VgaOutStream
{
public:
    int xpos = 0;
    int ypos = 0;

    const static char endl = '\n';

    VgaFormat format = {WHITE, BLACK};

    VgaOutStream &operator<<(const char c);
    VgaOutStream &operator<<(const char *c);

    VgaOutStream &operator<<(const int i);

    VgaOutStream &operator<<(const Color foreground);
    VgaOutStream &operator<<(const VgaFormat f);

    void clear();

private:
    static const int width = 80;
    static const int height = 25;
    volatile char *vgaBuffer = (volatile char *)0xB8000;

    void newline();
    void scroll();
    void setCursor(int row, int col);
};

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

static inline void outb(unsigned short port, unsigned char val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void VgaOutStream::setCursor(int row, int col)
{
    unsigned short pos = row * width + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

VgaOutStream vga = VgaOutStream();

void printLogo() {
    
    vga.clear();

    vga << vga.endl << vga.endl << vga.endl;
    vga << "                              " << RED << ",.=:!!t3Z3z.," << vga.endl ;
    vga << "                             " << RED << ":tt:::tt333EE3" << vga.endl ;
    vga << "                             " << RED << "Et:::ztt33EEEL " << GREEN << "@Ee.,      ..," << vga.endl ;
    vga << "                            " << RED << ";tt:::tt333EE7 " << GREEN << ";EEEEEEttttt33#" << vga.endl ;
    vga << "                           " << RED << ":Et:::zt333EEQ. " << GREEN << "$EEEEEttttt33QL" << vga.endl ;
    vga << "                           " << RED << "it::::tt333EEF " << GREEN << "@EEEEEEttttt33F" << vga.endl ;
    vga << "                          " << RED << ";3=*^```\"*4EEV " << GREEN << ":EEEEEEttttt33@." << vga.endl ;
    vga << "                          " << BLUE << ",.=::::!t=., ` " << GREEN << "@EEEEEEtttz33QF" << vga.endl ;
    vga << "                         " << BLUE << ";::::::::zt33)   " << GREEN << "\"4EEEtttji3P*" << vga.endl ;
    vga << "                        " << BLUE << ":t::::::::tt33.:" << YELLOW << "Z3z..  `` ,..g." << vga.endl ;
    vga << "                        " << BLUE << "i::::::::zt33F " << YELLOW << "AEEEtttt::::ztF" << vga.endl ;
    vga << "                       " << BLUE << ";:::::::::t33V " << YELLOW << ";EEEttttt::::t3" << vga.endl ;
    vga << "                       " << BLUE << "E::::::::zt33L " << YELLOW << "@EEEtttt::::z3F" << vga.endl ;
    vga << "                      " << BLUE << "{3=*^```\"*4E3) " << YELLOW << ";EEEtttt:::::tZ`" << vga.endl ;
    vga << "                                   " << BLUE << "` " << YELLOW << ":EEEEtttt::::z7" << vga.endl ;
    vga << "                                       " << YELLOW << "\"VEzjt:;;z>*`" << WHITE << vga.endl ;
    vga << vga.endl << vga.endl;
    vga << "                               -- Fenster -- " << vga.endl;
}