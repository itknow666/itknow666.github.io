# Timer_RotaryEncoder 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## Includes

```c
#include "stdio.h"
#include "string.h"
```

## 2

```c
HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);
```

## WHILE

```c
char send_buf[50] = {0};
  uint16_t cnt_encoder = 0;
  // 先设置TIM1的CNT值为1000，旋转编码器使得CNT值上下变化
  htim1.Instance->CNT = 1000;
  while (1)
  {
		// 如果CNT值发生变化，就打印出来
    if (cnt_encoder != __HAL_TIM_GET_COUNTER(&htim1))
    {
      cnt_encoder = __HAL_TIM_GET_COUNTER(&htim1);
      sprintf(send_buf, "%d\r\n", cnt_encoder);
      HAL_UART_Transmit(&huart2, (uint8_t *)send_buf, strlen(send_buf), 10);
    }
    HAL_Delay(10);
```

## 3

```c
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
