#ifndef VGA_H
#define VGA_H

#include <stdint.h>

class VgaOutStream;
namespace vga {
    extern VgaOutStream& out;
}

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

// Number format manipulators
enum class NumFormat {
    DEC,    // Decimal (default)
    HEX,    // Hexadecimal
    BIN,    // Binary
    SIZE    // Size
};

class VgaOutStream;

// Manipulator type
struct VgaManipulator {
    VgaOutStream& (*func)(VgaOutStream&);
};

class VgaOutStream
{
public:
    int xpos = 0;
    int ypos = 0;

    const static char endl = '\n';

    static VgaOutStream& instance();

    VgaFormat format = {WHITE, BLACK};
    NumFormat num_format = NumFormat::DEC;

    VgaOutStream &operator<<(const char c);
    VgaOutStream &operator<<(const char *c);

    VgaOutStream &operator<<(const int i);
    VgaOutStream &operator<<(const uint32_t i);
    VgaOutStream &operator<<(const uint64_t i);

    VgaOutStream &operator<<(const Color foreground);
    VgaOutStream &operator<<(const VgaFormat f);
    VgaOutStream &operator<<(VgaOutStream& (*manip)(VgaOutStream&));

    void clear();

private:
    static const int width = 80;
    static const int height = 25;
    static VgaOutStream instance_;
    volatile char *vgaBuffer = (volatile char *)0xB8000;

    VgaOutStream() {}

    void newline();
    void scroll();
    void setCursor(int row, int col);

    void print_hex(uint64_t value);
    void print_bin(uint64_t value);
    void print_size(uint64_t bytes);
};

// Manipulator functions
inline VgaOutStream& hex(VgaOutStream& stream) {
    stream.num_format = NumFormat::HEX;
    return stream;
}

inline VgaOutStream& dec(VgaOutStream& stream) {
    stream.num_format = NumFormat::DEC;
    return stream;
}

inline VgaOutStream& bin(VgaOutStream& stream) {
    stream.num_format = NumFormat::BIN;
    return stream;
};

inline VgaOutStream& size(VgaOutStream& stream) {
    stream.num_format = NumFormat::SIZE;
    return stream;
}

#endif // VGA_H