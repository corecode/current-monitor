#include <mchck.h>
#include "font.h"

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
SPI0_Handler(void)
{
        (void)SPI0->D;
}

enum {
        ADC_SAMPLES = 4096,
        ADC_SCALE = 1024,
        ADC2uA = (int)(1.2/4095*1000*1000/0.33*ADC_SCALE),
};

static volatile uint32_t adc_done;
static int64_t sample_accum;
static int sample_count;
static volatile int32_t sample_avg;

struct adc_calib {
        uint32_t scale;
        int32_t offset;
};

static struct adc_calib calib_fine = { ADC2uA/10000, 283*ADC2uA/10000 };
static struct adc_calib calib_coarse = { ADC2uA/10, 263*ADC2uA/10 };

void
adc_calibration_done(void *cbdata)
{
        adc_done = 1;
}

static void
adc_cb(struct adc_ctx *ctx, uint16_t val, int error, void *cbdata)
{
        struct adc_calib *calib = cbdata;
        int32_t scaled = val * calib->scale - calib->offset;

        if (scaled >= 300*ADC_SCALE) {
                adc_sample_start(&adc0_ctx, ADC_COARSE, adc_cb, &calib_coarse);
        } else {
                adc_sample_start(&adc0_ctx, ADC_FINE, adc_cb, &calib_fine);
        }

        sample_accum += scaled;
        sample_count++;
        if (sample_count == ADC_SAMPLES) {
                sample_avg = sample_accum / ADC_SAMPLES;
                adc_done = 1;
                sample_count = 0;
                sample_accum = 0;
        }
}

void
vref_init(void)
{
        bf_set_reg(SIM->SCGC4, SIM_SCGC4_VREF, 1);
        bf_set_reg(VREF->TRM, VREF_TRM_CHOPEN, 1);
        VREF->SC = VREF_SC_VREFEN_MASK |
                VREF_SC_ICOMPEN(1) |
                VREF_SC_MODE_LV(0b01);
        /* XXX better delay */
        for (volatile int i = 0; i < 15; i++)
                __NOP();
        bf_set_reg(VREF->SC, VREF_SC_REGEN, 1);
}

void
main(void)
{
        board_init();
        uart_init(&lpuart0);
        uart_set_baudrate(&lpuart0, 115200);
        /* uart_set_stdout(&lpuart0); */

        vref_init();

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

        for (volatile int i = 0; i < 5000; i++)
                __NOP();

        adc_init(&adc0_ctx);

        /* ignore calibration, we use our own factors */
        ADC0->PG = 0x8200;
        ADC0->OFS = 0;

        adc_sample_prepare(&adc0_ctx, ADC_MODE_AVG_32);
        ADC0->SC2 = ADC_SC2_REFSEL(0b01); /* use vref */

        adc_done = 0;
        adc_sample_start(&adc0_ctx, ADC_FINE, adc_cb, &calib_fine);

        display_value(0, 'm');

        for (;;) {
                __WFI();
                if (adc_done) {
                        if (sample_avg > 300*ADC_SCALE) {
                                display_value(sample_avg*10/1000/ADC_SCALE, 'm');
                        } else if (sample_avg < 0) {
                                display_value(0, LETTER_MU);
                        } else {
                                display_value(sample_avg*10/ADC_SCALE, LETTER_MU);
                        }
                        adc_done = 0;
                }
        }
}
