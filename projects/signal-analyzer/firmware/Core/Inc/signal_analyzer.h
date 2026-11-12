#ifndef SIGNAL_ANALYZER_H
#define SIGNAL_ANALYZER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SA_WAVEFORM_POINTS 128u

typedef struct {
    float code_at_zero_volts;
    float input_volts_per_code;
} sa_calibration_t;

typedef enum {
    SA_OK = 0,
    SA_ERR_ARGUMENT,
    SA_ERR_NO_SIGNAL,
    SA_ERR_CLIPPED
} sa_status_t;

typedef struct {
    float dc_volts;
    float vpp_volts;
    float frequency_hz;
    float rms_ac_volts;
    uint8_t clipped;
    uint8_t waveform[SA_WAVEFORM_POINTS]; /* 0..63, top to bottom */
} sa_result_t;

sa_calibration_t sa_calibration_from_two_points(
    float code_at_v1, float input_v1,
    float code_at_v2, float input_v2);

float sa_code_to_input_volts(uint16_t code, const sa_calibration_t *cal);

sa_status_t sa_measure_dc(
    const uint16_t *samples, size_t count,
    const sa_calibration_t *cal, sa_result_t *result);

sa_status_t sa_analyze_sine(
    const uint16_t *samples, size_t count, float sample_rate_hz,
    const sa_calibration_t *cal, sa_result_t *result);

/* Waveform-display mode: frequency uses the same crossing estimator, while
   Vpp uses trimmed extrema so square/triangle/arbitrary waves are not forced
   through the sine RMS relationship. */
sa_status_t sa_analyze_periodic_wave(
    const uint16_t *samples, size_t count, float sample_rate_hz,
    const sa_calibration_t *cal, sa_result_t *result);

void sa_make_waveform(
    const uint16_t *samples, size_t count, float sample_rate_hz,
    float frequency_hz, const sa_calibration_t *cal,
    uint8_t out_y[SA_WAVEFORM_POINTS]);

#ifdef __cplusplus
}
#endif

#endif
