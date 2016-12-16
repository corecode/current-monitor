#include <stdint.h>

struct glyph1610 {
        uint8_t encoding;
        uint16_t data[10];
};

struct font1610 {
        uint8_t count;
        struct glyph1610 glyphs[];
};

enum {
        LETTER_MU = 181,
};

extern const struct font1610 ter_u24b;
