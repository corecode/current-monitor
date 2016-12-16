#include <mchck.h>

void
board_init(void)
{
        pin_mode(PIN_UART_TX, PIN_MODE_MUX_ALT4);
        pin_mode(PIN_UART_RX, PIN_MODE_MUX_ALT4);
        pin_mode(PIN_LCD_DATA, PIN_MODE_MUX_GPIO);
        pin_mode(PIN_MOSI, PIN_MODE_MUX_ALT3);
        pin_mode(PIN_SCLK, PIN_MODE_MUX_ALT3);
        pin_mode(PIN_LCD_nCS, PIN_MODE_MUX_ALT3);
        pin_mode(PIN_LCD_LED, PIN_MODE_MUX_GPIO);
        pin_mode(PIN_BUTTON1, PIN_MODE_MUX_GPIO|PIN_MODE_PULLUP);
        pin_mode(PIN_BUTTON2, PIN_MODE_MUX_GPIO|PIN_MODE_PULLUP);
        /* analog pins are already in analog mode */

        lcd_led(0);
        gpio_dir(PIN_LCD_DATA, GPIO_OUTPUT);
        gpio_dir(PIN_LCD_LED, GPIO_OUTPUT);
        gpio_dir(PIN_BUTTON1, GPIO_OUTPUT);
        gpio_dir(PIN_BUTTON2, GPIO_INPUT);

        bf_set_reg(SIM_SCGC4, SIM_SCGC4_SPI0, 1); /* enable SPI clock */
        SPI0->C1 = SPI_C1_SPIE_MASK |
                SPI_C1_SPE_MASK |
                SPI_C1_MSTR_MASK |
                SPI_C1_CPOL_MASK |
                SPI_C1_CPHA_MASK |
                SPI_C1_SSOE_MASK;
        SPI0->C2 = SPI_C2_MODFEN_MASK;
        SPI0->BR = SPI_BR_SPPR(3-1) |
                SPI_BR_SPR(4);
        (void)SPI0->S;
        (void)SPI0->D;
}

void
lcd_led(int state)
{
        gpio_write(PIN_LCD_LED, !!state);
}
