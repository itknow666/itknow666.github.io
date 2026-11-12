#include "signal_analyzer.h"

#include <math.h>
#include <string.h>

#define SA_ADC_MAX_CODE 4095u
#define SA_ADC_CLIP_MARGIN 4u
#define SA_MIN_AC_RMS_VOLTS 0.010f

static float clampf(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

sa_calibration_t sa_calibration_from_two_points(
    float code_at_v1, float input_v1,
    float code_at_v2, float input_v2)
{
    sa_calibration_t cal = {0.0f, 0.0f};
    const float dc = code_at_v2 - code_at_v1;
    if (fabsf(dc) < 1.0e-6f) return cal;

    cal.input_volts_per_code = (input_v2 - input_v1) / dc;
    cal.code_at_zero_volts = code_at_v1 - input_v1 / cal.input_volts_per_code;
    return cal;
}

float sa_code_to_input_volts(uint16_t code, const sa_calibration_t *cal)
{
    if (cal == NULL) return 0.0f;
    return ((float)code - cal->code_at_zero_volts) * cal->input_volts_per_code;
}

static int valid_arguments(
    const uint16_t *samples, size_t count,
    const sa_calibration_t *cal, const sa_result_t *result)
{
    return samples != NULL && count >= 8u && cal != NULL && result != NULL &&
           cal->input_volts_per_code > 0.0f;
}

static float mean_code(const uint16_t *samples, size_t count)
{
    uint64_t sum = 0u;
    for (size_t i = 0u; i < count; ++i) sum += samples[i];
    return (float)sum / (float)count;
}

static uint8_t detect_clipping(const uint16_t *samples, size_t count)
{
    for (size_t i = 0u; i < count; ++i) {
        if (samples[i] <= SA_ADC_CLIP_MARGIN ||
            samples[i] >= SA_ADC_MAX_CODE - SA_ADC_CLIP_MARGIN) return 1u;
    }
    return 0u;
}

sa_status_t sa_measure_dc(
    const uint16_t *samples, size_t count,
    const sa_calibration_t *cal, sa_result_t *result)
{
    if (!valid_arguments(samples, count, cal, result)) return SA_ERR_ARGUMENT;
    memset(result, 0, sizeof(*result));

    /* Median of eight block means rejects isolated ADC/DMA spikes cheaply. */
    float means[8];
    const size_t block = count / 8u;
    for (size_t b = 0u; b < 8u; ++b) {
        uint64_t sum = 0u;
        const size_t begin = b * block;
        const size_t end = (b == 7u) ? count : begin + block;
        for (size_t i = begin; i < end; ++i) sum += samples[i];
        means[b] = (float)sum / (float)(end - begin);
    }
    for (size_t i = 1u; i < 8u; ++i) {
        const float key = means[i];
        size_t j = i;
        while (j > 0u && means[j - 1u] > key) {
            means[j] = means[j - 1u];
            --j;
        }
        means[j] = key;
    }

    const float robust_code = 0.5f * (means[3] + means[4]);
    result->dc_volts = (robust_code - cal->code_at_zero_volts) *
                       cal->input_volts_per_code;
    result->clipped = detect_clipping(samples, count);
    return result->clipped ? SA_ERR_CLIPPED : SA_OK;
}

static float estimate_frequency(
    const uint16_t *samples, size_t count, float sample_rate_hz,
    float center_code, float hysteresis_code)
{
    int armed = 0;
    size_t crossings = 0u;
    float first_cross = 0.0f;
    float last_cross = 0.0f;
    uint16_t previous = samples[0];

    for (size_t i = 1u; i < count; ++i) {
        const uint16_t current = samples[i];
        if ((float)current <= center_code - hysteresis_code) armed = 1;

        if (armed && (float)previous < center_code && (float)current >= center_code) {
            const float dy = (float)current - (float)previous;
            const float fraction = (dy > 0.0f) ?
                (center_code - (float)previous) / dy : 0.0f;
            const float crossing = (float)(i - 1u) + clampf(fraction, 0.0f, 1.0f);
            if (crossings == 0u) first_cross = crossing;
            last_cross = crossing;
            ++crossings;
            armed = 0;
        }
        previous = current;
    }

    if (crossings < 2u || last_cross <= first_cross) return 0.0f;
    return ((float)(crossings - 1u) * sample_rate_hz) /
           (last_cross - first_cross);
}

sa_status_t sa_analyze_sine(
    const uint16_t *samples, size_t count, float sample_rate_hz,
    const sa_calibration_t *cal, sa_result_t *result)
{
    if (!valid_arguments(samples, count, cal, result) || sample_rate_hz <= 0.0f)
        return SA_ERR_ARGUMENT;
    memset(result, 0, sizeof(*result));

    const float center = mean_code(samples, count);
    double energy = 0.0;
    uint16_t min_code = SA_ADC_MAX_CODE;
    uint16_t max_code = 0u;
    for (size_t i = 0u; i < count; ++i) {
        const float ac_code = (float)samples[i] - center;
        energy += (double)ac_code * (double)ac_code;
        if (samples[i] < min_code) min_code = samples[i];
        if (samples[i] > max_code) max_code = samples[i];
    }

    const float rms_code = sqrtf((float)(energy / (double)count));
    result->dc_volts = (center - cal->code_at_zero_volts) *
                       cal->input_volts_per_code;
    result->rms_ac_volts = rms_code * cal->input_volts_per_code;
    result->vpp_volts = 2.828427125f * result->rms_ac_volts;
    result->clipped = detect_clipping(samples, count);

    if (result->rms_ac_volts < SA_MIN_AC_RMS_VOLTS) return SA_ERR_NO_SIGNAL;

    const float span_code = (float)max_code - (float)min_code;
    const float five_mv_in_codes = 0.005f / cal->input_volts_per_code;
    const float hysteresis = fmaxf(five_mv_in_codes, 0.02f * span_code);
    result->frequency_hz = estimate_frequency(
        samples, count, sample_rate_hz, center, hysteresis);

    if (result->frequency_hz <= 0.0f) return SA_ERR_NO_SIGNAL;
    sa_make_waveform(samples, count, sample_rate_hz,
                     result->frequency_hz, cal, result->waveform);
    return result->clipped ? SA_ERR_CLIPPED : SA_OK;
}

sa_status_t sa_analyze_periodic_wave(
    const uint16_t *samples, size_t count, float sample_rate_hz,
    const sa_calibration_t *cal, sa_result_t *result)
{
    const sa_status_t status = sa_analyze_sine(
        samples, count, sample_rate_hz, cal, result);
    if (status == SA_ERR_ARGUMENT || status == SA_ERR_NO_SIGNAL) return status;

    enum { EXTREMA_COUNT = 8 };
    uint16_t low[EXTREMA_COUNT];
    uint16_t high[EXTREMA_COUNT];
    for (size_t i = 0u; i < EXTREMA_COUNT; ++i) {
        low[i] = SA_ADC_MAX_CODE;
        high[i] = 0u;
    }

    for (size_t i = 0u; i < count; ++i) {
        const uint16_t value = samples[i];
        if (value < low[EXTREMA_COUNT - 1u]) {
            low[EXTREMA_COUNT - 1u] = value;
            for (size_t j = EXTREMA_COUNT - 1u; j > 0u && low[j] < low[j - 1u]; --j) {
                const uint16_t swap = low[j];
                low[j] = low[j - 1u];
                low[j - 1u] = swap;
            }
        }
        if (value > high[EXTREMA_COUNT - 1u]) {
            high[EXTREMA_COUNT - 1u] = value;
            for (size_t j = EXTREMA_COUNT - 1u; j > 0u && high[j] > high[j - 1u]; --j) {
                const uint16_t swap = high[j];
                high[j] = high[j - 1u];
                high[j - 1u] = swap;
            }
        }
    }

    uint32_t low_sum = 0u;
    uint32_t high_sum = 0u;
    for (size_t i = 0u; i < EXTREMA_COUNT; ++i) {
        low_sum += low[i];
        high_sum += high[i];
    }
    const float robust_span_code = ((float)high_sum - (float)low_sum) /
                                   (float)EXTREMA_COUNT;
    result->vpp_volts = robust_span_code * cal->input_volts_per_code;
    return status;
}

void sa_make_waveform(
    const uint16_t *samples, size_t count, float sample_rate_hz,
    float frequency_hz, const sa_calibration_t *cal,
    uint8_t out_y[SA_WAVEFORM_POINTS])
{
    (void)cal;
    if (out_y == NULL) return;
    memset(out_y, 31, SA_WAVEFORM_POINTS);
    if (samples == NULL || count < 2u || sample_rate_hz <= 0.0f || frequency_hz <= 0.0f)
        return;

    float points_per_period = sample_rate_hz / frequency_hz;
    points_per_period = clampf(points_per_period, 4.0f, (float)(count - 1u));
    const float display_span = fminf(2.0f * points_per_period, (float)(count - 1u));

    uint16_t min_code = SA_ADC_MAX_CODE;
    uint16_t max_code = 0u;
    const size_t used = (size_t)display_span + 1u;
    for (size_t i = 0u; i < used; ++i) {
        if (samples[i] < min_code) min_code = samples[i];
        if (samples[i] > max_code) max_code = samples[i];
    }
    const float span = fmaxf(1.0f, (float)max_code - (float)min_code);

    for (size_t x = 0u; x < SA_WAVEFORM_POINTS; ++x) {
        const float source = display_span * (float)x / (float)(SA_WAVEFORM_POINTS - 1u);
        const size_t i0 = (size_t)source;
        const size_t i1 = (i0 + 1u < count) ? i0 + 1u : i0;
        const float frac = source - (float)i0;
        const float code = (1.0f - frac) * (float)samples[i0] + frac * (float)samples[i1];
        const float normalized = (code - (float)min_code) / span;
        out_y[x] = (uint8_t)(63.0f - 63.0f * clampf(normalized, 0.0f, 1.0f) + 0.5f);
    }
}
