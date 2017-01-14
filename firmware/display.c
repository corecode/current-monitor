#include <mchck.h>
#include "font.h"
#include "current-monitor.h"

enum lcd5110_mode {
        LCD5110_CMD = 0,
        LCD5110_DATA = 1,
};

enum lcd5110_instruction {
        LCD5110_FUN_SET = (1<<5),
        LCD5110_FUN_SET_PD = (1<<2),
        LCD5110_FUN_SET_V = (1<<1),
        LCD5110_FUN_SET_H = (1<<0),
        LCD5110_DISPLAY = (1<<3),
        LCD5110_DISPLAY_D = (1<<2),
        LCD5110_DISPLAY_E = (1<<0),
        LCD5110_SET_Y = (1<<6),
        LCD5110_SET_X = (1<<7),
        LCD5110_TEMP = (1<<2),
        LCD5110_BIAS = (1<<4),
        LCD5110_VOP = (1<<7),
};


void
SPI0_Handler(void)
{
        (void)SPI0->D;
}

void
lcd5110_send(uint8_t byte, enum lcd5110_mode data)
{
        gpio_write(PIN_LCD_DATA, data);
        while ((SPI0->S & SPI_S_SPTEF_MASK) == 0)
                /* NOTHING */;
        SPI0->D = byte;
        while ((SPI0->S & SPI_S_SPRF_MASK) == 0)
                /* NOTHING */;
        (void)SPI0->D;
}

void
lcd5110_goto(int x, int y)
{
        lcd5110_send(LCD5110_SET_X | x, LCD5110_CMD);
        lcd5110_send(LCD5110_SET_Y | y, LCD5110_CMD);
}

void
lcd5110_init(void)
{
        /* NVIC_EnableIRQ(SPI0_IRQn); */
        gpio_write(PIN_BUTTON1, 0);
        gpio_write(PIN_BUTTON1, 1);

        lcd_led(1);

        lcd5110_send(LCD5110_FUN_SET|LCD5110_FUN_SET_H, LCD5110_CMD);
        lcd5110_send(LCD5110_VOP | 0x3c, LCD5110_CMD);
        lcd5110_send(LCD5110_TEMP | 0, LCD5110_CMD);
        lcd5110_send(LCD5110_BIAS | 4, LCD5110_CMD);
        lcd5110_send(LCD5110_FUN_SET, LCD5110_CMD);
        for (int i = 0; i < 84*48/8; i++)
                lcd5110_send(0, LCD5110_DATA);
        lcd5110_send(LCD5110_DISPLAY|LCD5110_DISPLAY_D, LCD5110_CMD);
}

const struct glyph1610 *
font_find_glyph(int ch)
{
        const struct glyph1610 *glyph = NULL;
        for (int gidx = 0; gidx < ter_u24b.count; gidx++) {
                if (ter_u24b.glyphs[gidx].encoding == ch) {
                        glyph = &ter_u24b.glyphs[gidx];
                        break;
                }
        }

        return (glyph);
}

void
lcd5110_print_glyph(const struct glyph1610 *glyph, int row)
{
        for (int col = 0; col < 10; col++) {
                lcd5110_send((glyph->data[col] >> row) & 0xff, LCD5110_DATA);
        }
}

void
display_string(int x, int y, const char *str, int dotpos)
{
        for (int row = 0; row < 16; row += 8) {
                lcd5110_goto(x, y + row/8);
                for (const char *s = str; *s != 0; s++) {
                        const struct glyph1610 *glyph = font_find_glyph(*s);

                        lcd5110_print_glyph(glyph, row);

                        uint8_t spacedot = 0;
                        if (row == 8 && (s - str) == dotpos)
                                spacedot = 0b11100000;
                        lcd5110_send(spacedot, LCD5110_DATA);
                        lcd5110_send(spacedot, LCD5110_DATA);
                }
        }
}

void
display_value(uint32_t val, int unit)
{
        char str[7];
        size_t spos = 0;
        int leading_zero = 1;
        for (int pos = 100000; pos > 0; pos /= 10) {
                int digit = (val / pos) % 10;
                if (digit == 0 && leading_zero && pos > 10) {
                        str[spos++] = ' ';
                } else {
                        str[spos++] = digit + '0';
                        leading_zero = 0;
                }
        }
        str[spos++] = unit;
        display_string(0, 0, str, 4);
}

void
display_chart(chart c)
{
        for (int row = 0; row < CHART_HEIGHT; row += 8) {
                lcd5110_goto(0, (16+row)/8);
                for (int col = 0; col < CHART_WIDTH; col++) {
                        struct range *r = &c[col];

                        uint32_t pattern = 0;
                        for (int bit = 0; bit < 8; bit++) {
                                int pos = CHART_HEIGHT - (bit + row) - 1;
                                if (pos >= r->first && pos <= r->last)
                                        pattern |= 1 << bit;
                        }
                        lcd5110_send(pattern, LCD5110_DATA);
                }
        }
}
