#include "analyzer_app.h"

extern void platform_analog_startup(void);

/* Replace these two values with the measured ADC means at 0.000 V and 5.000 V. */
#define CAL_CODE_0V 2048.0f
#define CAL_CODE_5V 3909.0f

void run_analyzer_forever(void)
{
    platform_analog_startup();
    const sa_calibration_t cal = sa_calibration_from_two_points(
        CAL_CODE_0V, 0.0f, CAL_CODE_5V, 5.0f);

    analyzer_app_t app;
    analyzer_app_init(&app, cal);
    for (;;) analyzer_app_step(&app);
}
