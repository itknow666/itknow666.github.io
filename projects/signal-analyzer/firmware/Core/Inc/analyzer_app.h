#ifndef ANALYZER_APP_H
#define ANALYZER_APP_H

#include <stdint.h>

#include "signal_analyzer.h"

typedef enum {
    APP_MODE_DC = 0,
    APP_MODE_AC,
    APP_MODE_WAVEFORM
} app_mode_t;

typedef struct {
    app_mode_t mode;
    sa_calibration_t calibration;
    sa_result_t result;
    uint8_t result_valid;
} analyzer_app_t;

/* Platform functions implemented in stm32_port.c. */
int platform_capture_adc(uint32_t sample_rate_hz, uint16_t *buffer, uint16_t count);
uint8_t platform_button_pressed(void);
void platform_oled_measurement(app_mode_t mode, const sa_result_t *result, uint8_t valid);
void platform_oled_waveform(const sa_result_t *result, uint8_t valid);
void platform_delay_ms(uint32_t milliseconds);

void analyzer_app_init(analyzer_app_t *app, sa_calibration_t calibration);
void analyzer_app_step(analyzer_app_t *app);

#endif
