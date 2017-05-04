#include <mchck.h>
#include "font.h"
#include "current-monitor.h"
#include <math.h>

enum {
        ADC_SAMPLES = 4096,
};

#define VREF_V 1.19
#define ADC_COUNTS 65535
#define SENSE_R 0.33
#define BASE_GAIN (VREF_V/SENSE_R/ADC_COUNTS)
#define FIXPOINT_SCALE (1000000000UL)
#define FIXPOINT_SCALE_U (FIXPOINT_SCALE/1000000)
#define FIXPOINT_SCALE_M (FIXPOINT_SCALE/1000)

static volatile uint32_t adc_done;
static int64_t sample_accum;
static int sample_count;
static volatile int32_t sample_avg;

struct adc_calib {
        float scale;
        float offset;
        float thres;
};

static struct adc_calib calib_fine = { BASE_GAIN/1000, 33e-6, 3.5e-3 };
static struct adc_calib calib_coarse = { BASE_GAIN/10, 1.818e-3, 3e-3 };

void
adc_calibration_done(void *cbdata)
{
        adc_done = 1;
}

static void
adc_cb(struct adc_ctx *ctx, uint16_t val, int error, void *cbdata)
{
        struct adc_calib *calib = cbdata;
        float scaled = val * calib->scale - calib->offset;

        if (scaled >= calib->thres) {
                adc_sample_start(&adc0_ctx, ADC_COARSE, adc_cb, &calib_coarse);
        } else {
                adc_sample_start(&adc0_ctx, ADC_FINE, adc_cb, &calib_fine);
        }

        sample_accum += scaled * FIXPOINT_SCALE;
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


static chart history;

static float scale(uint32_t val)
{
        float fval = val;
        float alpha = FIXPOINT_SCALE * 10;
        return log(fval + sqrt(fval*fval + alpha*alpha));
}

void
main(void)
{
        board_init();
        uart_init(&lpuart0);
        uart_set_baudrate(&lpuart0, 115200);
        /* uart_set_stdout(&lpuart0); */

        vref_init();
        st7565r_init();

        for (volatile int i = 0; i < 5000; i++)
                __NOP();

        adc_init(&adc0_ctx);

        /* ignore calibration, we use our own factors */
        ADC0->PG = 0x8200;
        ADC0->OFS = 0;

        adc_sample_prepare(&adc0_ctx, ADC_MODE_AVG_32);
        //ADC0->SC2 = ADC_SC2_REFSEL(0b01); /* use vref */

        adc_done = 0;
        adc_sample_start(&adc0_ctx, ADC_FINE, adc_cb, &calib_fine);

        display_value(0, 'm');

        float scale_top = scale(0.32*FIXPOINT_SCALE);
        float scale_bottom = scale(0);
        float scale_range = scale_top - scale_bottom;

        for (;;) {
                __WFI();
                if (adc_done) {
                        if (sample_avg > 3500*FIXPOINT_SCALE_U) {
                                display_value((sample_avg*10 + FIXPOINT_SCALE_M-1)/FIXPOINT_SCALE_M, 'm');
                        } else if (sample_avg < 0) {
                                display_value(0, LETTER_MU);
                        } else {
                                display_value((sample_avg*10 + FIXPOINT_SCALE_U-1)/FIXPOINT_SCALE_U, LETTER_MU);
                        }
                        adc_done = 0;
                        memcpy(&history[0], &history[1], sizeof(history)-sizeof(*history));

                        float scaled = scale(sample_avg);
                        int pos = CHART_HEIGHT * (scaled - scale_bottom) / scale_range;
                        history[CHART_WIDTH-1].first = history[CHART_WIDTH-1].last = pos;
                        display_chart(history);
                }
        }
}
