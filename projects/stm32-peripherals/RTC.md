# RTC 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## Includes

```c
#include "kk_rtc.h"
#include <stdio.h>
#include <string.h>
```

## 2

```c
char message[50] = "";
  struct tm *now;
```

## WHILE

```c
while (1)
  {
		now = KK_RTC_GetTime();
	  sprintf(message, "%d-%d-%d %02d:%02d:%02d", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
				now->tm_hour, now->tm_min, now->tm_sec);
	  HAL_UART_Transmit(&huart2, (uint8_t*)message, strlen(message), HAL_MAX_DELAY);
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
