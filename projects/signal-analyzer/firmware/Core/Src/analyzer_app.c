#include "analyzer_app.h"

#include <string.h>

#define FAST_SAMPLE_RATE_HZ 200000u
#define SLOW_SAMPLE_RATE_HZ 20000u
#define CAPTURE_SAMPLES 2048u
#define BUTTON_DEBOUNCE_MS 25u

static uint16_t capture_buffer[CAPTURE_SAMPLES];

void analyzer_app_init(analyzer_app_t *app, sa_calibration_t calibration)
{
    if (app == NULL) return;
    memset(app, 0, sizeof(*app));
    app->mode = APP_MODE_DC;
    app->calibration = calibration;
}

static void poll_button(analyzer_app_t *app)
{
    if (!platform_button_pressed()) return;
    platform_delay_ms(BUTTON_DEBOUNCE_MS);
    if (!platform_button_pressed()) return;

    app->mode = (app_mode_t)(((unsigned)app->mode + 1u) % 3u);
    app->result_valid = 0u;
    while (platform_button_pressed()) platform_delay_ms(5u);
}

void analyzer_app_step(analyzer_app_t *app)
{
    if (app == NULL) return;
    poll_button(app);

    if (app->mode == APP_MODE_DC) {
        if (platform_capture_adc(SLOW_SAMPLE_RATE_HZ, capture_buffer, 512u) != 0) {
            app->result_valid = 0u;
        } else {
            const sa_status_t status = sa_measure_dc(
                capture_buffer, 512u, &app->calibration, &app->result);
            app->result_valid = (status == SA_OK);
        }
        platform_oled_measurement(app->mode, &app->result, app->result_valid);
        return;
    }

    sa_status_t status = SA_ERR_NO_SIGNAL;
    if (platform_capture_adc(FAST_SAMPLE_RATE_HZ, capture_buffer, CAPTURE_SAMPLES) == 0) {
        status = (app->mode == APP_MODE_WAVEFORM) ?
            sa_analyze_periodic_wave(capture_buffer, CAPTURE_SAMPLES,
                                     (float)FAST_SAMPLE_RATE_HZ,
                                     &app->calibration, &app->result) :
            sa_analyze_sine(capture_buffer, CAPTURE_SAMPLES,
                            (float)FAST_SAMPLE_RATE_HZ,
                            &app->calibration, &app->result);
    }

    /* A 10.24 ms fast window cannot guarantee two rising edges at 100 Hz.
       Retry at 20 kS/s only when the fast acquisition finds no period. */
    if (status == SA_ERR_NO_SIGNAL &&
        platform_capture_adc(SLOW_SAMPLE_RATE_HZ, capture_buffer, CAPTURE_SAMPLES) == 0) {
        status = (app->mode == APP_MODE_WAVEFORM) ?
            sa_analyze_periodic_wave(capture_buffer, CAPTURE_SAMPLES,
                                     (float)SLOW_SAMPLE_RATE_HZ,
                                     &app->calibration, &app->result) :
            sa_analyze_sine(capture_buffer, CAPTURE_SAMPLES,
                            (float)SLOW_SAMPLE_RATE_HZ,
                            &app->calibration, &app->result);
    }

    app->result_valid = (status == SA_OK);
    if (app->mode == APP_MODE_WAVEFORM)
        platform_oled_waveform(&app->result, app->result_valid);
    else
        platform_oled_measurement(app->mode, &app->result, app->result_valid);
}
