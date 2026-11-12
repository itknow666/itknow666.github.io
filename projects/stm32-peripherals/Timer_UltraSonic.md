# Timer_UltraSonic 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## Includes

```c
#include "stdio.h"
#include "string.h"
#include "oled.h"
```

## PV

```c
uint8_t echo_flag = 0;
uint16_t rising_cnt = 0, falling_cnt = 0;
char buf_uart[32] = {0};
char buf_oled[30] = {0};
```

## 2

```c
HAL_Delay(20);
  OLED_Init();
```

## WHILE

```c
while (1)
  {
		// 触发
	  HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, 1);
	  HAL_Delay(5);
	  HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, 0);
	  // 清零
	  rising_cnt = 0;
	  falling_cnt = 0;
	  echo_flag = 0;
	  __HAL_TIM_SET_COUNTER(&htim1, 0);
	  // 开始捕获
	  __HAL_TIM_SET_CAPTUREPOLARITY(&htim1, TIM_CHANNEL_3, TIM_INPUTCHANNELPOLARITY_RISING);
	  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_3);
	  // 等待完成
	 for (uint16_t i = 0; i < 150; i++)
	 {
	   if (rising_cnt != 0 && falling_cnt != 0)
	   {
		 // 计算距离
		 // 定时器每1us计数1次，因此 距离=计数*0.34/2（毫米）
		 float distance = (falling_cnt - rising_cnt) * 0.017;
		 // 构造字符串
		 sprintf(buf_uart, "Distance: %.2f cm\r\n", distance);
		 sprintf(buf_oled, "%.2f cm", distance);
		 break;
	   }
	   HAL_Delay(1);
	 }
	 // 串口发送数据
	 HAL_UART_Transmit(&huart2, (uint8_t *)buf_uart, strlen(buf_uart), 1000);
	 // oled显示数据
	 OLED_NewFrame();
	 OLED_PrintString(32, 0, "Distance", &font16x16, OLED_COLOR_NORMAL);
	 OLED_PrintString(32, 26, "NULL", &font16x16, OLED_COLOR_NORMAL);    // 未连接显示NULL
	 OLED_PrintString(30, 26, buf_oled, &font16x16, OLED_COLOR_NORMAL);
	 OLED_ShowFrame();
	// 停止捕获
	HAL_TIM_IC_Stop_IT(&htim1, TIM_CHANNEL_3);
	HAL_Delay(500);
```

## 3

```c
}
```

## 4

```c
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &htim1 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
  {
    // 捕获到上升沿
    if (!echo_flag)
    {
      rising_cnt = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
      echo_flag = 1;
      __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_3, TIM_INPUTCHANNELPOLARITY_FALLING);
    }
    // 捕获到下降沿
    else
    {
      falling_cnt = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
      echo_flag = 0;
      __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_3, TIM_INPUTCHANNELPOLARITY_RISING);
    }
  }
}
```

## Error_Handler_Debug

```c
/* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
```

## 6

```c
/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
```
