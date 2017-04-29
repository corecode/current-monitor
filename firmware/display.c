#include <mchck.h>
#include "font.h"
#include "current-monitor.h"

enum st7565r_mode {
        ST7565R_CMD = 0,
        ST7565R_DATA = 1,
};

enum {
        ST7565R_ROWS = 64,
        ST7565R_COLUMNS = 128,
};

#define ST7565R_DISPLAY_ON(x) (0b10101110|!!(x))
#define ST7565R_STARTLINE(x) (0b01000000|(x))
#define ST7565R_PAGE(x) (0b10110000|(x))
#define ST7565R_COLUMN(msb, x) (0b00000000|(!!(msb) << 4)|(x))
#define ST7565R_ADC(x) (0b10100000|!!(x))
#define ST7565R_REVERSE(x) (0b10100110|!!(x))
#define ST7565R_ALLON(x) (0b10100100|!!(x))
#define ST7565R_BIAS(x) (0b10100010|!!(x))
#define ST7565R_RESET 0b11100010
#define ST7565R_COMREVERSE(x) (0b11000000|(!!(x) << 3))
#define ST7565R_POWER(x) (0b00101000|(x))
#define ST7565R_RESISTOR(x) (0b00100000|x)
#define ST7565R_ELECVOL 0b10000001
#define ST7565R_STATIND(x) (0b10101100|!!x)
#define ST7565R_BOOSTER 0b11111000
#define ST7565R_NOP 0b11100011


void
SPI0_Handler(void)
{
        (void)LCD_SPI->DL;
}

void
st7565r_send(uint8_t byte, enum st7565r_mode data)
{
        while ((LCD_SPI->S & SPI_S_SPTEF_MASK) == 0)
                /* NOTHING */;
        gpio_write(PIN_LCD_DATA, data);
        LCD_SPI->DL = byte;
        while ((LCD_SPI->S & SPI_S_SPRF_MASK) == 0)
                /* NOTHING */;
        (void)LCD_SPI->DL;
}

void
st7565r_cmd(uint8_t byte)
{
        st7565r_send(byte, ST7565R_CMD);
}

void
st7565r_data(uint8_t byte)
{
        st7565r_send(byte, ST7565R_DATA);
}

void
st7565r_contrast(uint8_t val)
{
        st7565r_cmd(ST7565R_ELECVOL);
        st7565r_cmd(val);
}

void
st7565r_goto(int x, int y)
{
        st7565r_cmd(ST7565R_PAGE(y/8));
        st7565r_cmd(ST7565R_COLUMN(0, x & 0xf));
        st7565r_cmd(ST7565R_COLUMN(1, x >> 4));
}

static void
delay(int ms)
{
        for (volatile int i = 0; i < ms * 10000; i++)
                __NOP();
}

void
st7565r_init(void)
{
        pin_mode(PIN_LCD_nCS, PIN_MODE_MUX_GPIO);
        gpio_write(PIN_LCD_nCS, 0);
        gpio_write(PIN_LCD_nRES, 0);
        delay(500);
        lcd_led(1);

        gpio_write(PIN_LCD_nRES, 1);
        st7565r_cmd(ST7565R_BIAS(1));
        st7565r_cmd(ST7565R_ADC(0));
        st7565r_cmd(ST7565R_COMREVERSE(0));
        st7565r_cmd(ST7565R_STARTLINE(0));
        st7565r_cmd(ST7565R_POWER(4));
        delay(50);
        st7565r_cmd(ST7565R_POWER(6));
        delay(50);
        st7565r_cmd(ST7565R_POWER(7));
        delay(50);
        st7565r_cmd(ST7565R_RESISTOR(6));
        st7565r_cmd(ST7565R_REVERSE(0));
        st7565r_contrast(0x18);
        st7565r_cmd(ST7565R_DISPLAY_ON(1));
        delay(100);
        st7565r_cmd(ST7565R_ALLON(1));
        delay(200);
        st7565r_cmd(ST7565R_ALLON(0));
        pin_mode(PIN_LCD_nCS, PIN_MODE_MUX_ALT2|PIN_MODE_SLEW_SLOW);

        for (;;);

        st7565r_cmd(ST7565R_RESET);

        /* NVIC_EnableIRQ(SPI0_IRQn); */
        gpio_write(PIN_LCD_nRES, 1);
        /* XXX delay 1ms */
        for (volatile int i = 0; i < 20000; i++)
                __NOP();
        gpio_write(PIN_LCD_nRES, 0);
        /* XXX delay 1ms */
        for (volatile int i = 0; i < 20000; i++)
                __NOP();
        gpio_write(PIN_LCD_nRES, 1);
        /* XXX delay 1ms */
        for (volatile int i = 0; i < 200000; i++)
                __NOP();

        st7565r_cmd(ST7565R_ADC(0));
        st7565r_cmd(ST7565R_COMREVERSE(1));
        st7565r_cmd(ST7565R_BIAS(0));
        st7565r_cmd(ST7565R_POWER(7));
        st7565r_cmd(ST7565R_RESISTOR(6));
        st7565r_contrast(0x16);
        st7565r_cmd(ST7565R_STARTLINE(0));
        st7565r_cmd(ST7565R_DISPLAY_ON(1));

        for (int page = 0; page < ST7565R_ROWS/8; page++) {
                st7565r_goto(0, page*8);
                for (int col = 0; col < ST7565R_COLUMNS; col++) {
                        st7565r_data(0);
                }
        }
        st7565r_goto(0, 0);

        st7565r_cmd(ST7565R_ALLON(1));
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
st7565r_print_glyph(const struct glyph1610 *glyph, int row)
{
        for (int col = 0; col < 10; col++) {
                st7565r_data((glyph->data[col] >> row) & 0xff);
        }
}

void
display_string(int x, int y, const char *str, int dotpos)
{
        for (int row = 0; row < 16; row += 8) {
                st7565r_goto(x, y + row/8);
                for (const char *s = str; *s != 0; s++) {
                        const struct glyph1610 *glyph = font_find_glyph(*s);

                        st7565r_print_glyph(glyph, row);

                        uint8_t spacedot = 0;
                        if (row == 8 && (s - str) == dotpos)
                                spacedot = 0b11100000;
                        st7565r_data(spacedot);
                        st7565r_data(spacedot);
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
                st7565r_goto(0, (16+row)/8);
                for (int col = 0; col < CHART_WIDTH; col++) {
                        struct range *r = &c[col];

                        uint32_t pattern = 0;
                        for (int bit = 0; bit < 8; bit++) {
                                int pos = CHART_HEIGHT - (bit + row) - 1;
                                if (pos >= r->first && pos <= r->last)
                                        pattern |= 1 << bit;
                        }
                        st7565r_data(pattern);
                }
        }
}
