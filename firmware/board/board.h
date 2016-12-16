enum {
        PIN_UART_TX = PIN_PTA3,
        PIN_UART_RX = PIN_PTA4,
        PIN_LCD_DATA = GPIO_PTA5,
        PIN_MOSI = PIN_PTA7,
        PIN_SCLK = PIN_PTB0,
        PIN_LCD_nCS = PIN_PTB10,
        PIN_LCD_LED = GPIO_PTB11,
        PIN_BUTTON1 = GPIO_PTB6,
        PIN_BUTTON2 = GPIO_PTB7,
        PIN_VREF = PIN_PTB2,
        ADC_FINE = 2,
        ADC_COARSE = 0,
        ADC_VBAT = 8,
};

void board_init(void);
void lcd_led(int state);
