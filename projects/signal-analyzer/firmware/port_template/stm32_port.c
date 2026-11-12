/*
 * Copy this file into a CubeMX STM32F103C8T6 project after configuring:
 *   ADC1 IN0 + external TIM3 TRGO + DMA normal/half-word
 *   TIM3 master trigger = update event
 *   PB12 input pull-up
 * Replace the four ui_* hooks with calls to the SSD1306 library in use.
 */
#include "main.h"
#include "adc.h"
#include "gpio.h"
#include "tim.h"

#include "analyzer_app.h"

#define TIMER3_CLOCK_HZ 72000000u
#define CAPTURE_TIMEOUT_MS 300u

static volatile uint8_t adc_capture_done;

/* Implement these in the OLED module. */
extern void ui_show_measurement(app_mode_t mode, const sa_result_t *result, uint8_t valid);
extern void ui_show_waveform(const sa_result_t *result, uint8_t valid);

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) adc_capture_done = 1u;
}

int platform_capture_adc(uint32_t sample_rate_hz, uint16_t *buffer, uint16_t count)
{
    if (sample_rate_hz == 0u || buffer == NULL || count == 0u) return -1;
    const uint32_t ticks = TIMER3_CLOCK_HZ / sample_rate_hz;
    if (ticks == 0u || ticks > 65536u) return -2;

    (void)HAL_TIM_Base_Stop(&htim3);
    (void)HAL_ADC_Stop_DMA(&hadc1);
    __HAL_TIM_SET_PRESCALER(&htim3, 0u);
    __HAL_TIM_SET_AUTORELOAD(&htim3, ticks - 1u);
    __HAL_TIM_SET_COUNTER(&htim3, 0u);
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    adc_capture_done = 0u;

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)buffer, count) != HAL_OK) return -3;
    if (HAL_TIM_Base_Start(&htim3) != HAL_OK) {
        (void)HAL_ADC_Stop_DMA(&hadc1);
        return -4;
    }

    const uint32_t started = HAL_GetTick();
    while (!adc_capture_done) {
        if ((HAL_GetTick() - started) > CAPTURE_TIMEOUT_MS) {
            (void)HAL_TIM_Base_Stop(&htim3);
            (void)HAL_ADC_Stop_DMA(&hadc1);
            return -5;
        }
    }
    (void)HAL_TIM_Base_Stop(&htim3);
    (void)HAL_ADC_Stop_DMA(&hadc1);
    return 0;
}

uint8_t platform_button_pressed(void)
{
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET;
}

void platform_oled_measurement(app_mode_t mode, const sa_result_t *result, uint8_t valid)
{
    ui_show_measurement(mode, result, valid);
}

void platform_oled_waveform(const sa_result_t *result, uint8_t valid)
{
    ui_show_waveform(result, valid);
}

void platform_delay_ms(uint32_t milliseconds)
{
    HAL_Delay(milliseconds);
}

/* Call once after MX_*_Init(), before analyzer_app_init(). */
void platform_analog_startup(void)
{
    (void)HAL_ADCEx_Calibration_Start(&hadc1);
}
