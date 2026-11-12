#ifndef BOARD_H
#define BOARD_H
#include "stm32f1xx_hal.h"
#include "measurement.h"
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim2,htim3;
extern UART_HandleTypeDef huart1;
extern uint16_t adc_pairs[MEAS_SAMPLES*2];
enum { KEY_MODE_SHORT=1u,KEY_HOLD_SHORT=2u,KEY_MODE_LONG=4u,KEY_HOLD_LONG=8u };
void board_init(void);
void board_tick_1ms(void);
uint32_t board_keys(void);
HAL_StatusTypeDef board_acquire(uint32_t fs);
void board_acquire_stop(void);
int board_frame_ready(void);
void board_normalize_frame(void);
int board_acquisition_error(void);
float board_frequency(void);
void board_frequency_reset(void);
float board_vref_read(void);
int board_cal_load(Calibration *c);
int board_cal_save(Calibration *c);
int board_uart_getchar(void);
void board_uart_puts(const char *s);
void Error_Handler(void);
#endif
