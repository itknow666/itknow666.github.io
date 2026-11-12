# I2C_SOFT_keil 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## Includes

```c
#include "aht20.h"
#include <stdio.h>
#include <string.h>
```

## 2

```c
AHT20_Init();

  float temperature = 0.0, humidity = 0.0;
  char message[50];
```

## WHILE

```c
while (1)
  {
		AHT20_Read(&temperature, &humidity);

    sprintf(message, "温度: %.1f ℃，湿度: %.1f %%\r\n", temperature, humidity);
    HAL_UART_Transmit(&huart2, (uint8_t *)message, strlen(message),100);

    HAL_Delay(1000);
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
