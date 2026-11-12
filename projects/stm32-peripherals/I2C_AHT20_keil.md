# I2C_AHT20_keil 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## Includes

```c
#include "aht20.h"
#include "stdio.h"
#include "string.h"
#include "oled.h"
```

## 2

```c
float temperature, humidity; // 温度和湿度变量
  char message_uart[50];       // 要发送的字符串
  char message_temp[30];	// oled显示温度字符串
  char message_hum[30];		// oled显示湿度字符串
  AHT20_Init();
  HAL_Delay(20);
  OLED_Init();
```

## WHILE

```c
while (1)
  {
		// 读取温湿度
	  AHT20_Read(&temperature, &humidity);
	  // 打印温湿度
	  sprintf(message_uart, "温度: %.1f ℃, 湿度: %.1f %%\r\n", temperature, humidity);
	  sprintf(message_temp, "温度: %.1f ℃", temperature);
	  sprintf(message_hum, "湿度: %.1f %%", humidity);
	  // 串口发送
	  HAL_UART_Transmit(&huart2, (uint8_t *)message_uart, strlen(message_uart), HAL_MAX_DELAY);
	  // oled显示
	  OLED_NewFrame();
	  OLED_PrintString(0, 16, message_temp, &font16x16, OLED_COLOR_NORMAL);
	  OLED_PrintString(0, 32, message_hum, &font16x16, OLED_COLOR_NORMAL);
	  OLED_ShowFrame();
	  // 延时 1 秒
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
