struct range {
        uint8_t first, last;
};

enum {
        CHART_WIDTH = 76,
        CHART_HEIGHT = 32,
};
typedef struct range chart[CHART_WIDTH];

void lcd5110_init(void);
void display_string(int x, int y, const char *str, int dotpos);
void display_value(uint32_t val, int unit);
void display_chart(chart c);
