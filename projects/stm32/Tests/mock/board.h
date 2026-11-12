#ifndef TEST_BOARD_H
#define TEST_BOARD_H
#include "measurement.h"
typedef enum {HAL_OK=0,HAL_ERROR=1} HAL_StatusTypeDef;
typedef enum {GPIO_PIN_RESET=0,GPIO_PIN_SET=1} GPIO_PinState;
#define GPIOB 1
#define GPIOC 2
#define GPIO_PIN_1 2
#define GPIO_PIN_13 8192
enum {KEY_MODE_SHORT=1u,KEY_HOLD_SHORT=2u,KEY_MODE_LONG=4u,KEY_HOLD_LONG=8u};
extern uint16_t adc_pairs[MEAS_SAMPLES*2];
uint32_t HAL_GetTick(void);
GPIO_PinState HAL_GPIO_ReadPin(int port,int pin);
void HAL_GPIO_WritePin(int port,int pin,GPIO_PinState value);
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
#endif
