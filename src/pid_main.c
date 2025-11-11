#include <stdio.h>

#include "pico/stdio.h"
#include "pico/time.h"
#include "pico/util/queue.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#include "pid.h"

#define ADC_CHAN 1
#define ADC_PIN (26 + ADC_CHAN)

// guarda las muestras para PID
uint16_t data_buf[3] = {0};
// indica si hay datos nuevos
volatile bool data_flag = false;
// indica si puede imprimir
volatile bool print_flag = false;

char print_buf[256] = {0};
repeating_timer_t print_timer;

const uint pwm_pin = 22;
// 125 MHz system clk
// freq PWM = sys_clk / period
// PWM period = (TOP+1) * DIV
// 3 kHz values
const float pwm_div = 66.f;
const uint16_t pwm_wrap = 624;

// funcion periodica - habilita la impresion
bool cb_enable_print(repeating_timer_t *rt)
{
    print_flag = true;

    return true;
}

// Guarda la ultima lectura del ADC
void irq_handler_adc()
{
    uint16_t value = adc_fifo_get();

    // check error bit
    if (value & (1 << 15))
    {
        // si hubo error no se habilita la flag para no procesar
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
    const float adc_freq = 1e3f;
    const float clk_div = (48e6f / adc_freq) - 1;
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
    pid_init_k(1.f / adc_freq);

    // start ADCs
    adc_fifo_drain();
    adc_run(true);
    // start PWM
    pwm_set_enabled(pwm_slice, true);

    float pid_out = 0.f;
    uint16_t pwm_level = 0;
    pwm_set_gpio_level(pwm_pin, pwm_level);

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
        pid_out = pid_process(pid_out, data_buf[0]);

        // convierte el valor de 12 bits al rango de niveles del PWM ([0-624])
        // (redondea en vez de truncar)
        // ( X * (TOP+1) + 2048 ) / 4096
        // pwm_level = ( pid_out * (pwm_wrap + 1) + 2048 ) / 4096;
        pwm_level = (uint16_t)pid_out;

        // el redondeo puede resultar en un nivel de 625 en el caso extremo cuando el pid pide 4095
        // if (pwm_level > pwm_wrap)
        //     pwm_level = pwm_wrap;

        // invertir PWM (por inversion de hardware)
        pwm_level = pwm_wrap - pwm_level;
            
        // actualizar pwm
        pwm_set_gpio_level(pwm_pin, pwm_level);

        // shift
        data_buf[2] = data_buf[1];
        data_buf[1] = data_buf[0];
    }

    return 0;
}