#include <stdint.h>

// #define PID_MAX 4095
#define PID_MAX 624
#define PID_MIN 0

volatile uint16_t set_point = ((uint32_t)PID_MAX * 1 / 2);
// const uint16_t set_point = (500)

const float Kp = 1.f;
const float Ki = 3.f;
const float Kd = 0.f;

// float Kp = 0.04;
// float Ki = 0.3;
// float Kd = 0;


float K1 = 0;
float K2 = 0;
float K3 = 0;

float err0 = 0.f;
float err1 = 0.f;
float err2 = 0.f;

static void pid_init_k(float T)
{
    K1 = (2.0f * T * Kp + Ki * T * T + 2.0f * Kd) / (2.0f * T);
    K2 = (Ki * T * T - 2.0f * Kp * T - 4.0f * Kd) / (2.0f * T);
    K3 = Kd / T;
}

// setpoint [0-100]
static void pid_update_setpoint(uint16_t sp_100)
{
    set_point = (uint32_t)PID_MAX * sp_100 / 100;
}

static uint16_t pid_get_setpoint()
{
    return (uint32_t)set_point * 100 / PID_MAX;
}

// xxxxx procesa los valores de 12 bits directamente del ADC
// procesa los valores a nivel de pwm
static float pid_process(float old_duty, uint16_t adc12)
{
    // version adc 12 bits
    uint16_t meas = ((uint32_t)adc12 * (PID_MAX + 1) + 2048) >> 12;
    // version adc 8 bits
    // adc12 = ((uint32_t)adc12 * (PID_MAX + 1) + 128) / 256;
    if (meas > PID_MAX)
        meas = PID_MAX;

    err0 = set_point - meas;
    float A = K1 * err0;
    float B = K2 * err1;
    float C = K3 * err2;

    float u = A + B + C + old_duty;

    // saturacion
    if (u > PID_MAX)
        u = PID_MAX;
    else if (u < PID_MIN)
        u = PID_MIN;

    // shift
    err2 = err1;
    err1 = err0;

    return u;
}