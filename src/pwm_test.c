#include <stdio.h>

#include "pico/stdio.h"
#include "pico/time.h"
#include "pico/util/queue.h"

#include "hardware/adc.h"
#include "hardware/pwm.h"

#define ADC_CHAN 2
#define ADC_PIN (26 + ADC_CHAN)

#define ADC_VREF 3.3f

#define NUM_SAMPLES 3
uint16_t data_buf[NUM_SAMPLES] = {0};
volatile bool data_flag = false;
volatile bool print_flag = false;

char print_buf[256] = {0};
repeating_timer_t print_timer;

repeating_timer_t pwm_timer;

const uint pwm_pin = 22;
// 125 MHz system clk
// freq PWM = sys_clk / period
// PWM period = (TOP+1) * DIV
// 20 kHz values
// const float pwm_div = 10.f;
// 10 kHz values
// const float pwm_div = 20.f;
// 6 kHz values
// const float pwm_div = 33.f;
// 5 kHz values
// const float pwm_div = 40.f;
// 4 kHz values
// const float pwm_div = 50.f;
// 3 kHz values
// pico_w
// const float pwm_div = 66.f;
// pico2_w
const float pwm_div = 80.f;
// 2.5 kHz values
// const float pwm_div = 80.f;
// 2 kHz values
// const float pwm_div = 100.f;
// 1 kHz values
// const float pwm_div = 200.f;
// 0.5 kHz values
// const float pwm_div = 300.f;
// const uint16_t pwm_wrap = 624*2;
const uint16_t pwm_wrap = 624;
volatile uint16_t pwm_level = 624;

bool cb_enable_print(repeating_timer_t *rt)
{
    print_flag = true;

    return true;
}

bool cb_update_pwm(repeating_timer_t *rt)
{
    static int i = 9;

    // if (i > 6)
    // {
    //     pwm_level = ((uint32_t)pwm_wrap * 9) / 10;
    // }
    // else
    // {
    pwm_level = ((uint32_t)pwm_wrap * i) / 10;
    // }

    if (pwm_level > pwm_wrap)
        pwm_level = pwm_wrap;

    i -= 1;
    if (i < 0)
        i = 9;

    return true;
}

void irq_handler_adc()
{
    uint16_t value = adc_fifo_get();

    // check error bit
    if (value & 1 << 15)
    {
        return;
    }

    data_buf[0] = value;
    data_flag = true;
}

int main()
{
    stdio_init_all();
    sleep_ms(3000);
    printf("hello world!\n");

    // printf timer
    add_repeating_timer_ms(1200, cb_enable_print, NULL, &print_timer);
    // pwm update timer
    add_repeating_timer_ms(6000, cb_update_pwm, NULL, &pwm_timer);

    // configure PWM
    gpio_set_function(pwm_pin, GPIO_FUNC_PWM);
    uint pwm_slice = pwm_gpio_to_slice_num(pwm_pin);

    pwm_config pwm_cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&pwm_cfg, pwm_div);
    pwm_config_set_wrap(&pwm_cfg, pwm_wrap);

    pwm_init(pwm_slice, &pwm_cfg, false);

    // configure ADC
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHAN);

    /*  adc starts a conversion every clkdiv + 1 clock cycles
        adc is paced by 48 MHz clock
        adc should sample every x + 1 cycles
        for x=47999 -> clkdiv=x+1=48e3 -> sample freq is 1ksps -> fs = 1 kHz */
    int clk_div = 47999;
    adc_set_clkdiv(clk_div);

    adc_fifo_setup(
        true,  // Write each completed conversion to the sample FIFO
        false, // Enable DMA data request (DREQ)
        1,     // DREQ (and IRQ) asserted when at least 1 sample present
        true,  // sets bit 15 if conversion error occurs (bits are numbered [31:0])
        false  // Don't! Shift each sample to 8 bits when pushing to FIFO
    );

    // ADC interrupt - configure NVIC (cpu-side)
    irq_set_enabled(ADC_IRQ_FIFO, true);
    irq_set_exclusive_handler(ADC_IRQ_FIFO, irq_handler_adc);

    // ADC interrupt - configure IRQ (adc-side)
    adc_irq_set_enabled(true);

    // --- Startup -----
    // start ADCs
    adc_run(true);
    // start PWM
    pwm_set_enabled(pwm_slice, true);
    // pwm_set_gpio_level(pwm_pin, pwm_level);
    pwm_set_gpio_level(pwm_pin, pwm_wrap/2);

    while (1)
    {
        if (!data_flag)
        {
            if (print_flag)
            {
                print_flag = false;
                printf("ADC %u - %u - %u\n", data_buf[0], data_buf[1], data_buf[2]);
                printf("PWM level %u - duty %u\n", pwm_level, (pwm_level * 100) / pwm_wrap);
            }
            continue;
        }
        data_flag = false;

        // procesar datos
        // prueba - pwm proporcional (redondea en vez de truncar)
        // ( ADC * (TOP+1) + 2048 ) / 4096
        // pwm_level = ((uint32_t)data_buf[0] * (pwm_wrap + 1) + 2048) / 4096;

        // el redondeo puede resultar en un nivel de 625 en el caso extremo cuando el ADC lee 4095
        // if (pwm_level > pwm_wrap)
        //     pwm_level = pwm_wrap;

        pwm_set_gpio_level(pwm_pin, pwm_level);

        // shift
        for (size_t i = NUM_SAMPLES - 1; i > 0; i--)
        {
            data_buf[i] = data_buf[i - 1];
        }
    }

    return 0;
}