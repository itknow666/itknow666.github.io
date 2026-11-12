#ifndef MEASUREMENT_H
#define MEASUREMENT_H
#include <stdint.h>
#include <stddef.h>

#define MEAS_SAMPLES 1024u
#define DC_POINTS_MAX 17u
#define AC_POINTS 4u
#define CAL_MAGIC 0x4243414cu
#define CAL_VERSION 1u
#define MAIN_GAIN 0.25179856115f
#define MAIN_OFFSET_V 1.85179856115f
#define SMALL_GAIN 2.51798312f
#define ADC_FULL_SCALE 4095.0f
#define NOMINAL_VDDA_MV 3300.0f

typedef struct { float code, mv; } DcPoint;
typedef struct {
    uint32_t magic, version, generation, dc_count;
    DcPoint dc[DC_POINTS_MAX];
    float ac_trim[2][AC_POINTS];
    uint32_t ac_valid[2];
    float frequency_trim, vref_at_cal;
    uint32_t oled_type;
    uint32_t crc;
} Calibration;

enum { MEAS_FREQ_OK=1u, MEAS_MAIN_CLIP=2u, MEAS_SMALL_CLIP=4u,
       MEAS_NO_SIGNAL=8u, MEAS_DISTORTED=16u, MEAS_OVER_RANGE=32u };
typedef struct {
    float mean[2], raw_pp[2], stddev[2], fitted_pp[2];
    uint16_t min[2], max[2];
    float dc_mv, vpp_mv, frequency_hz, residual_ratio;
    uint32_t flags, sample_rate;
    uint8_t channel;
} Measurement;
typedef struct { int16_t low[128], high[128], center[128]; float periods; } Waveform;

extern const float ac_frequencies[AC_POINTS];
void calibration_defaults(Calibration *c);
uint32_t calibration_crc(const Calibration *c);
int calibration_valid(const Calibration *c);
const Calibration *calibration_select(const Calibration *a,const Calibration *b);
int calibration_add_dc(Calibration *c, float raw_code, float mv, float vref);
float calibration_dc(const Calibration *c, float code, float vref);
float calibration_ac_factor(const Calibration *c, uint8_t channel, float hz, float vref);
float small_gain_at(float hz);
float estimate_frequency(const uint16_t *pairs, size_t n, uint8_t channel, float fs);
void measure_frame(const uint16_t *pairs, size_t n, uint32_t fs, float captured_hz,
                   float vref, const Calibration *cal, int sine_mode, Measurement *out);
void measure_waveform(const uint16_t *pairs, size_t n, const Measurement *m,
                      const Calibration *cal, float vref, Waveform *wave);
#endif
