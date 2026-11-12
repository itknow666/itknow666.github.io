#include "signal_analyzer.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PI_F 3.14159265358979323846f
#define N 2048u

static uint16_t samples[N];
static uint32_t noise_state = 1u;

static float uniform_noise(void)
{
    noise_state = noise_state * 1664525u + 1013904223u;
    return ((float)((noise_state >> 8) & 0xffffu) / 65535.0f) - 0.5f;
}

static uint16_t input_voltage_to_code(float input_voltage)
{
    const float adc_voltage = 0.3f * input_voltage + 1.65f;
    float code = adc_voltage * 4095.0f / 3.3f + 0.35f * uniform_noise();
    if (code < 0.0f) code = 0.0f;
    if (code > 4095.0f) code = 4095.0f;
    return (uint16_t)(code + 0.5f);
}

static int expect_close(const char *name, float actual, float expected, float tolerance)
{
    const float error = fabsf(actual - expected);
    printf("%-22s actual=%10.5f expected=%10.5f error=%9.5f\n",
           name, actual, expected, error);
    if (error > tolerance) {
        fprintf(stderr, "FAIL: %s tolerance %.5f exceeded\n", name, tolerance);
        return 1;
    }
    return 0;
}

static int test_dc(sa_calibration_t cal, float volts)
{
    for (size_t i = 0u; i < N; ++i) samples[i] = input_voltage_to_code(volts);
    sa_result_t result;
    const sa_status_t status = sa_measure_dc(samples, N, &cal, &result);
    if (status != SA_OK) return 1;
    char label[40];
    snprintf(label, sizeof(label), "DC %.1f V", volts);
    return expect_close(label, result.dc_volts, volts, 0.010f);
}

static int test_sine(sa_calibration_t cal, float fs, float frequency, float vpp)
{
    for (size_t i = 0u; i < N; ++i) {
        const float volts = 0.5f * vpp * sinf(2.0f * PI_F * frequency * (float)i / fs);
        samples[i] = input_voltage_to_code(volts);
    }
    sa_result_t result;
    const sa_status_t status = sa_analyze_sine(samples, N, fs, &cal, &result);
    if (status != SA_OK) {
        fprintf(stderr, "FAIL: sine status=%d for %.1f Hz\n", (int)status, frequency);
        return 1;
    }
    char label[40];
    snprintf(label, sizeof(label), "F %.0f Hz", frequency);
    int failed = expect_close(label, result.frequency_hz, frequency, 0.02f * frequency);
    snprintf(label, sizeof(label), "Vpp %.2f V", vpp);
    failed |= expect_close(label, result.vpp_volts, vpp, 0.03f * vpp + 0.005f);
    for (size_t i = 0u; i < SA_WAVEFORM_POINTS; ++i)
        if (result.waveform[i] > 63u) failed = 1;
    return failed;
}

static int test_square_wave(sa_calibration_t cal)
{
    const float fs = 200000.0f;
    const float frequency = 5000.0f;
    const float vpp = 3.0f;
    for (size_t i = 0u; i < N; ++i) {
        const float phase = fmodf(frequency * (float)i / fs, 1.0f);
        samples[i] = input_voltage_to_code(phase < 0.5f ? -0.5f * vpp : 0.5f * vpp);
    }
    sa_result_t result;
    const sa_status_t status = sa_analyze_periodic_wave(samples, N, fs, &cal, &result);
    if (status != SA_OK) return 1;
    int failed = expect_close("Square F 5 kHz", result.frequency_hz, frequency, 0.02f * frequency);
    failed |= expect_close("Square Vpp 3 V", result.vpp_volts, vpp, 0.02f * vpp);
    return failed;
}

int main(void)
{
    const float code_0v = input_voltage_to_code(0.0f);
    const float code_5v = input_voltage_to_code(5.0f);
    const sa_calibration_t cal = sa_calibration_from_two_points(code_0v, 0.0f,
                                                                code_5v, 5.0f);
    int failed = 0;
    failed |= test_dc(cal, -5.0f);
    failed |= test_dc(cal, 0.0f);
    failed |= test_dc(cal, 5.0f);
    failed |= test_sine(cal, 20000.0f, 100.0f, 0.10f);
    failed |= test_sine(cal, 200000.0f, 1000.0f, 0.50f);
    failed |= test_sine(cal, 200000.0f, 10000.0f, 2.00f);
    failed |= test_sine(cal, 200000.0f, 20000.0f, 5.00f);
    failed |= test_square_wave(cal);
    puts(failed ? "TESTS FAILED" : "ALL TESTS PASSED");
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
