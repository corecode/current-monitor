enum {
        PIN_UART_RX = PIN_PTA1,
        PIN_UART_TX = PIN_PTA2,
        PIN_USR1 = PIN_PTA18,
        PIN_USR2 = PIN_PTA19,
        ADC_FINE = 8,
        ADC_COARSE = 9,
        ADC_VBAT = 11,
        PIN_LCD_LED = GPIO_PTC4,
        PIN_BUTTON1 = GPIO_PTB5,
        PIN_BUTTON2 = GPIO_PTB6,
        PIN_LCD_DATA = GPIO_PTB7,
        PIN_LCD_nCS = PIN_PTD4,
        PIN_LCD_nCS_PCS = 0,
        PIN_SCLK = PIN_PTD5,
        PIN_MOSI = PIN_PTD6,
        PIN_LCD_nRES = PIN_PTD7,
        PIN_VREF = PIN_PTE30,
};

#define LCD_SPI SPI1

void board_init(void);
void lcd_led(int state);
