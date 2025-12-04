#include <stdint.h>

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
    BIN     // Binary
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
    volatile char *vgaBuffer = (volatile char *)0xB8000;

    void newline();
    void scroll();
    void setCursor(int row, int col);

    void print_hex(uint64_t value);
    void print_bin(uint64_t value);
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
